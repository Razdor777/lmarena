#pragma once
//
// PngWriter — минимальный PNG-энкодер (RGBA8, без интерлейса) своими руками.
//
// Почему свой: тянуть в проект zlib/libpng из-за одной задачи (сохранить грани
// кубемапы) не стоит — это лишний вес в DLL, которая и так инжектится в игру.
// Здесь deflate реализован на фиксированном Huffman + LZ77 (hash-chain): по
// спеке это допустимый поток, его поймёт любой декодер, а на картинках вроде
// звёздного неба (много нулей и плавных градиентов) он даёт кратное сжатие.
//
// Формат куска PNG — стандартный:
//   сигнатура | IHDR | IDAT (zlib: 0x78 0x01 + deflate + adler32) | IEND
// Строки фильтруются фильтром 0 (None): для звёздного поля это не хуже, а
// лишний код фильтрации только добавляет риск ошибки.
//
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace PngWriter
{
    namespace Detail
    {
        // ── CRC32 (PNG) ────────────────────────────────────────────────────
        // Таблица собирается один раз и как локальный static-объект: его
        // инициализацию компилятор делает потокобезопасной сам (magic statics).
        // Раньше здесь стоял ручной флаг `static bool built` — он давал гонку:
        // два потока могли собирать таблицу одновременно. Для генерации неба
        // это неважно (работает один поток), но такой «почти безопасный» код
        // прощает ошибку ровно до того момента, когда кто-то вызовет writer из
        // второго потока.
        inline const uint32_t* crcTable()
        {
            static const std::vector<uint32_t> table = []()
            {
                std::vector<uint32_t> t(256);
                for (uint32_t n = 0; n < 256; ++n)
                {
                    uint32_t c = n;
                    for (int k = 0; k < 8; ++k)
                        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                    t[n] = c;
                }
                return t;
            }();
            return table.data();
        }

        inline uint32_t crc32(const uint8_t* data, size_t length, uint32_t crc = 0xFFFFFFFFu)
        {
            const uint32_t* table = crcTable();
            for (size_t i = 0; i < length; ++i)
                crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
            return crc;
        }

        inline uint32_t adler32(const uint8_t* data, size_t length)
        {
            uint32_t a = 1, b = 0;
            for (size_t i = 0; i < length; ++i)
            {
                a = (a + data[i]) % 65521u;
                b = (b + a) % 65521u;
            }
            return (b << 16) | a;
        }

        // ── Поток бит: deflate пишет биты в байт с младшего ────────────────
        struct BitWriter
        {
            std::vector<uint8_t>& out;
            uint32_t buffer = 0;
            int bits = 0;

            explicit BitWriter(std::vector<uint8_t>& target) : out(target) {}

            void putBits(uint32_t value, int count)
            {
                buffer |= (value & ((1u << count) - 1u)) << bits;
                bits += count;
                while (bits >= 8)
                {
                    out.push_back(static_cast<uint8_t>(buffer & 0xFF));
                    buffer >>= 8;
                    bits -= 8;
                }
            }

            // Коды Хаффмана идут СТАРШИМ битом вперёд, поэтому значение
            // разворачиваем: putBits пишет с младшего.
            void putCode(uint32_t code, int count)
            {
                uint32_t reversed = 0;
                for (int i = 0; i < count; ++i)
                    reversed = (reversed << 1) | ((code >> i) & 1u);
                putBits(reversed, count);
            }

            void flush()
            {
                if (bits > 0)
                {
                    out.push_back(static_cast<uint8_t>(buffer & 0xFF));
                    buffer = 0;
                    bits = 0;
                }
            }
        };

        // ── Таблицы для длины/дистанции (RFC 1951, 3.2.5) ─────────────────
        inline const int kLenBase[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                          35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
        inline const int kLenExtra[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                           3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
        inline const int kDistBase[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
                                           193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
                                           6145, 8193, 12289, 16385, 24577 };
        inline const int kDistExtra[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
                                            6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

        // Литерал по фиксированным кодам (RFC 1951, 3.2.6)
        inline void emitLiteral(BitWriter& bw, uint8_t value)
        {
            if (value < 144) bw.putCode(0x30u + value, 8);
            else             bw.putCode(0x190u + (value - 144), 9);
        }

        inline void emitLength(BitWriter& bw, int length)
        {
            int index = 28;
            while (index > 0 && kLenBase[index] > length) --index;

            const int symbol = 257 + index;
            if (symbol <= 279) bw.putCode(static_cast<uint32_t>(symbol - 256), 7);
            else               bw.putCode(0xC0u + static_cast<uint32_t>(symbol - 280), 8);

            const int extra = kLenExtra[index];
            if (extra > 0) bw.putBits(static_cast<uint32_t>(length - kLenBase[index]), extra);
        }

        inline void emitDistance(BitWriter& bw, int distance)
        {
            int index = 29;
            while (index > 0 && kDistBase[index] > distance) --index;

            bw.putCode(static_cast<uint32_t>(index), 5);
            const int extra = kDistExtra[index];
            if (extra > 0) bw.putBits(static_cast<uint32_t>(distance - kDistBase[index]), extra);
        }

        // ── deflate с фиксированным Huffman и поиском совпадений ──────────
        inline std::vector<uint8_t> deflateFixed(const std::vector<uint8_t>& src)
        {
            std::vector<uint8_t> out;
            out.reserve(src.size() / 2 + 64);

            BitWriter bw(out);
            bw.putBits(1, 1);   // BFINAL = 1
            bw.putBits(1, 2);   // BTYPE  = 01 (fixed Huffman), в поток идёт младшим битом

            constexpr size_t kWindow = 32768;
            constexpr int kMaxChain = 24;

            const size_t n = src.size();
            std::vector<int32_t> head(1u << 15, -1);
            std::vector<int32_t> prev(n, -1);

            auto hash3 = [&](size_t i) -> uint32_t
            {
                return ((static_cast<uint32_t>(src[i]) << 10) ^
                        (static_cast<uint32_t>(src[i + 1]) << 5) ^
                        static_cast<uint32_t>(src[i + 2])) & 0x7FFFu;
            };

            size_t i = 0;
            while (i < n)
            {
                int bestLength = 0;
                int bestDistance = 0;

                if (i + 3 <= n)
                {
                    const uint32_t h = hash3(i);
                    int32_t candidate = head[h];
                    int chain = 0;

                    const size_t maxLength = (n - i < 258) ? (n - i) : 258;

                    while (candidate >= 0 && chain++ < kMaxChain &&
                           (i - static_cast<size_t>(candidate)) <= kWindow)
                    {
                        size_t length = 0;
                        const size_t from = static_cast<size_t>(candidate);
                        while (length < maxLength && src[from + length] == src[i + length]) ++length;

                        if (length > static_cast<size_t>(bestLength))
                        {
                            bestLength = static_cast<int>(length);
                            bestDistance = static_cast<int>(i - from);
                            if (bestLength >= 258) break;
                        }
                        candidate = prev[from];
                    }

                    prev[i] = head[h];
                    head[h] = static_cast<int32_t>(i);
                }

                if (bestLength >= 3)
                {
                    emitLength(bw, bestLength);
                    emitDistance(bw, bestDistance);

                    // Промежуточные позиции тоже кладём в цепочки, иначе
                    // следующий поиск не увидит эти совпадения.
                    for (size_t k = i + 1; k < i + static_cast<size_t>(bestLength) && k + 3 <= n; ++k)
                    {
                        const uint32_t h = hash3(k);
                        prev[k] = head[h];
                        head[h] = static_cast<int32_t>(k);
                    }

                    i += static_cast<size_t>(bestLength);
                }
                else
                {
                    emitLiteral(bw, src[i]);
                    ++i;
                }
            }

            bw.putCode(0, 7);    // end of block: символ 256 → код 0, 7 бит
            bw.flush();
            return out;
        }

        inline std::vector<uint8_t> zlibStream(const std::vector<uint8_t>& raw)
        {
            // 0x78 0x01: CM=8, окно 32K, FLEVEL=0; (0x7801 % 31 == 0, как требует спека)
            std::vector<uint8_t> out;
            out.push_back(0x78);
            out.push_back(0x01);

            const auto body = deflateFixed(raw);
            out.insert(out.end(), body.begin(), body.end());

            const uint32_t adler = adler32(raw.data(), raw.size());
            out.push_back(static_cast<uint8_t>((adler >> 24) & 0xFF));
            out.push_back(static_cast<uint8_t>((adler >> 16) & 0xFF));
            out.push_back(static_cast<uint8_t>((adler >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(adler & 0xFF));
            return out;
        }

        inline void pushU32(std::vector<uint8_t>& out, uint32_t value)
        {
            out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
            out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(value & 0xFF));
        }

        inline void pushChunk(std::vector<uint8_t>& out, const char type[4],
                              const std::vector<uint8_t>& data)
        {
            pushU32(out, static_cast<uint32_t>(data.size()));

            const size_t crcStart = out.size();
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), data.begin(), data.end());

            const uint32_t crc = crc32(out.data() + crcStart, out.size() - crcStart) ^ 0xFFFFFFFFu;
            pushU32(out, crc);
        }
    } // namespace Detail

    // rgba — width*height*4 байт, строки сверху вниз.
    inline std::vector<uint8_t> encodeRGBA(int width, int height, const uint8_t* rgba)
    {
        using namespace Detail;

        std::vector<uint8_t> file;
        file.reserve(static_cast<size_t>(width) * height / 2 + 1024);

        const uint8_t signature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
        file.insert(file.end(), signature, signature + 8);

        std::vector<uint8_t> ihdr;
        pushU32(ihdr, static_cast<uint32_t>(width));
        pushU32(ihdr, static_cast<uint32_t>(height));
        ihdr.push_back(8);    // bit depth
        ihdr.push_back(6);    // colour type: RGBA
        ihdr.push_back(0);    // compression: deflate
        ihdr.push_back(0);    // filter: adaptive
        ihdr.push_back(0);    // interlace: none
        pushChunk(file, "IHDR", ihdr);

        const size_t stride = static_cast<size_t>(width) * 4;
        std::vector<uint8_t> raw;
        raw.resize((stride + 1) * static_cast<size_t>(height));
        for (int y = 0; y < height; ++y)
        {
            uint8_t* row = raw.data() + static_cast<size_t>(y) * (stride + 1);
            row[0] = 0;   // фильтр None
            std::memcpy(row + 1, rgba + static_cast<size_t>(y) * stride, stride);
        }

        pushChunk(file, "IDAT", zlibStream(raw));
        pushChunk(file, "IEND", {});
        return file;
    }

    inline bool writeRGBA(const std::filesystem::path& file, int width, int height,
                          const uint8_t* rgba)
    {
        const auto bytes = encodeRGBA(width, height, rgba);

        std::error_code ec;
        std::filesystem::create_directories(file.parent_path(), ec);

        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
        return out.good();
    }
}
