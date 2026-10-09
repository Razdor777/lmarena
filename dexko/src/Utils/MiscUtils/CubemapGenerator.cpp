#include "CubemapGenerator.hpp"

#include "PngWriter.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include <spdlog/spdlog.h>

// ── ПОЧЕМУ У ВНУТРЕННИХ ИМЁН ПРЕФИКС `sky` ───────────────────────────────────
// Проект собирается с CMAKE_UNITY_BUILD, то есть несколько .cpp склеиваются в
// одну единицу трансляции. Имена из АНОНИМНОГО пространства имён при этом
// попадают в глобальную область видимости, и если такое же имя есть ещё где-то в
// той же склейке, компилятор отказывается выбирать:
//
//     error: reference to 'Vec3' is ambiguous
//
// Проверено компилятором на мини-проекте с той же unity-настройкой
// (.freebuff/tmp-unity), а не по памяти. Важная деталь: ломается не код ВНУТРИ
// нашего пространства имён, а те места, где на наши сущности ссылается
// именованное пространство ниже (`CubemapGenerator`): поиск идёт оттуда и видит
// обоих кандидатов. Замена анонимного пространства имён на именованное НЕ
// лечит — любая using-директива снова выносит имена в ближайшее общее
// пространство имён, то есть в глобальное (тоже проверено компилятором).
// Поэтому имена сделаны уникальными: `Vec3` есть в AutoPath.hpp, `kPi` — в
// ModernDropdown.cpp и ещё пяти модулях, `normalize`/`dot` — в glm, `fbm`
// встречается в шейдерах. С префиксом `sky` совпадение невозможно, а разбивка
// батчей (она меняется при добавлении файлов) больше ни на что не влияет.
namespace
{
    constexpr float kSkyPi = 3.14159265358979323846f;

    struct SkyVec3
    {
        float x = 0.f, y = 0.f, z = 0.f;
    };

    SkyVec3 skyNormalize(const SkyVec3& v)
    {
        const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (length <= 1e-6f) return { 0.f, 1.f, 0.f };
        return { v.x / length, v.y / length, v.z / length };
    }

    float skyDot(const SkyVec3& a, const SkyVec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    // ── Хеш ────────────────────────────────────────────────────────────────
    // splitmix64: дешёвый, детерминированный, без таблиц. Один и тот же сид
    // даёт одно и то же небо на любой машине.
    uint32_t skyHashU32(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
    {
        auto mix = [](uint64_t v)
        {
            v *= 0xBF58476D1CE4E5B9ull;
            v ^= v >> 31;
            v *= 0x94D049BB133111EBull;
            v ^= v >> 29;
            return v;
        };

        uint64_t h = 0x9E3779B97F4A7C15ull;
        h = mix(h ^ x);
        h = mix(h ^ y);
        h = mix(h ^ z);
        h = mix(h ^ seed);
        return static_cast<uint32_t>(h >> 32);
    }

    float skyHashFloat(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
    {
        return static_cast<float>(skyHashU32(x, y, z, seed)) * (1.0f / 4294967296.0f);
    }

    // ── Value noise + fBm (3D, значит без швов на гранях) ─────────────────
    float skyValueNoise(const SkyVec3& p, uint32_t seed)
    {
        const float fx = std::floor(p.x), fy = std::floor(p.y), fz = std::floor(p.z);
        const int ix = static_cast<int>(fx), iy = static_cast<int>(fy), iz = static_cast<int>(fz);

        float tx = p.x - fx, ty = p.y - fy, tz = p.z - fz;
        // smoothstep: убирает «решётку» из линейной интерполяции
        tx = tx * tx * (3.f - 2.f * tx);
        ty = ty * ty * (3.f - 2.f * ty);
        tz = tz * tz * (3.f - 2.f * tz);

        auto corner = [&](int dx, int dy, int dz)
        {
            return skyHashFloat(static_cast<uint32_t>(ix + dx), static_cast<uint32_t>(iy + dy),
                             static_cast<uint32_t>(iz + dz), seed);
        };

        const float c000 = corner(0, 0, 0), c100 = corner(1, 0, 0);
        const float c010 = corner(0, 1, 0), c110 = corner(1, 1, 0);
        const float c001 = corner(0, 0, 1), c101 = corner(1, 0, 1);
        const float c011 = corner(0, 1, 1), c111 = corner(1, 1, 1);

        const float x00 = c000 + (c100 - c000) * tx;
        const float x10 = c010 + (c110 - c010) * tx;
        const float x01 = c001 + (c101 - c001) * tx;
        const float x11 = c011 + (c111 - c011) * tx;

        const float y0 = x00 + (x10 - x00) * ty;
        const float y1 = x01 + (x11 - x01) * ty;

        return y0 + (y1 - y0) * tz;
    }

    float skyFbm(SkyVec3 p, uint32_t seed, int octaves, float frequency, float gain)
    {
        float sum = 0.f, amplitude = 1.f, norm = 0.f;
        for (int i = 0; i < octaves; ++i)
        {
            sum += amplitude * skyValueNoise({ p.x * frequency, p.y * frequency, p.z * frequency },
                                          seed + static_cast<uint32_t>(i) * 7919u);
            norm += amplitude;
            amplitude *= gain;
            frequency *= 2.f;
        }
        return norm > 0.f ? sum / norm : 0.f;
    }

    // ── Звёзды ─────────────────────────────────────────────────────────────
    // Звёзды ставятся в сетке по (азимут, полярный угол), то есть по сферическим
    // координатам направления. Так они гарантированно совпадают на рёбрах граней,
    // а не «дорисовываются» дважды. Единственный артефакт метода — у полюсов
    // ячейки физически мельче, поэтому там чуть плотнее; для космического неба
    // это незаметно.
    struct SkyStarLayer
    {
        float cellsPerFace; // ячеек на грань: задаёт размер звезды в пикселях
        float probability;  // доля ячеек, где вообще есть звезда
        float baseRadius;   // радиус свечения в долях ячейки
        float radiusSpread;
        float brightness;
    };

    // Масштаб сетки. Ячейка задана в ДОЛЯХ ГРАНИ, поэтому при любом разрешении
    // звёзды выходят одного видимого размера. Два подвоха, на которые здесь уже
    // наступили:
    //   1. сетка без масштаба (=2.6 ячейки на всё небо) даёт звезду размером в
    //      треть неба;
    //   2. слишком частые ячейки дают стену из звёзд: при вероятности 0.8 и
    //      ячейке в 1.5 пикселя светлыми оказывалось ~28% неба.
    // Сейчас: ячейка ≈ faceSize вокруг экватора (≈4 px), вероятность малая —
    // всего по трём слоям выходит ~4 тысячи звёзд на небо.
    float skyStarField(float uu, float vv, int layerIndex, uint32_t seed,
                    float densityScale, int faceSize)
    {
        static const SkyStarLayer layers[3] = {
            { 1.00f, 0.100f, 0.10f, 0.10f, 0.38f },   // редкая мелкая звёздная пыль
            { 0.60f, 0.060f, 0.13f, 0.12f, 0.90f },   // средние звёзды
            { 0.30f, 0.048f, 0.16f, 0.14f, 1.70f },   // редкие яркие
        };

        const SkyStarLayer& layer = layers[layerIndex];
        const float probability = std::min(0.60f, layer.probability * densityScale);

        const float cellCount = std::max(16.f, layer.cellsPerFace * static_cast<float>(faceSize));
        const float cu = uu * cellCount;
        const float cv = vv * (cellCount * 0.5f);

        const int baseU = static_cast<int>(std::floor(cu));
        const int baseV = static_cast<int>(std::floor(cv));

        const uint32_t layerSeed = seed + static_cast<uint32_t>(layerIndex) * 104729u;

        float glow = 0.f;
        for (int du = -1; du <= 1; ++du)
        {
            for (int dv = -1; dv <= 1; ++dv)
            {
                const int cellU = baseU + du;
                const int cellV = baseV + dv;

                if (skyHashFloat(static_cast<uint32_t>(cellU), static_cast<uint32_t>(cellV),
                              layerSeed, 131u) > probability)
                    continue;

                const float starU = static_cast<float>(cellU) +
                                    skyHashFloat(static_cast<uint32_t>(cellU), static_cast<uint32_t>(cellV),
                                              layerSeed, 7u);
                const float starV = static_cast<float>(cellV) +
                                    skyHashFloat(static_cast<uint32_t>(cellU), static_cast<uint32_t>(cellV),
                                              layerSeed, 11u);

                const float dx = cu - starU;
                const float dy = cv - starV;
                const float distance = std::sqrt(dx * dx + dy * dy);

                const float radius = layer.baseRadius +
                                     layer.radiusSpread * skyHashFloat(static_cast<uint32_t>(cellU),
                                                                    static_cast<uint32_t>(cellV),
                                                                    layerSeed, 13u);
                const float t = distance / radius;
                if (t > 2.6f) continue;

                // Ядро + узкое гало: так звезда выглядит точкой с лёгким сиянием,
                // а не пятном. Широкое гало здесь уже пробовали — из-за него в
                // гистограмме появлялись блобы по 10 px и больше.
                glow += std::exp(-2.4f * t * t) + 0.10f * std::exp(-1.30f * t * t);
            }
        }

        return glow * layer.brightness;
    }

    // Цвет звезды по хешу: от голубовато-белых до оранжевых, как в реальном небе.
    SkyVec3 skyStarColor(uint32_t seed)
    {
        static const SkyVec3 palette[5] = {
            { 0.62f, 0.74f, 1.00f },   // голубая
            { 0.80f, 0.88f, 1.00f },   // бело-голубая
            { 1.00f, 1.00f, 1.00f },   // белая
            { 1.00f, 0.94f, 0.78f },   // жёлтая
            { 1.00f, 0.72f, 0.52f },   // оранжевая
        };

        const float pick = skyHashFloat(seed, 0x51u, 0x17u, 3u);
        const int index = static_cast<int>(pick * 5.f) % 5;
        return palette[index];
    }

    // ── Само небо по направлению ──────────────────────────────────────────
    SkyVec3 skySampleColor(const SkyVec3& dir, const CubemapGenerator::Settings& settings)
    {
        const uint32_t seed = settings.seed;

        // Полоса Млечного Пути. Наклон и ширина — вкусовые, но именно она даёт
        // «космос»: без неё небо выглядит просто чёрным с точками.
        const SkyVec3 bandNormal = skyNormalize({ 0.32f, 0.86f, -0.39f });
        const float bandDistance = skyDot(dir, bandNormal);
        const float bandWidth = 0.17f;
        const float band = std::exp(-(bandDistance * bandDistance) / (2.f * bandWidth * bandWidth));
        const float bandTexture = 0.35f + 1.35f * skyFbm({ dir.x, dir.y, dir.z }, seed + 555u, 5, 3.1f, 0.55f);

        // Туманности: тот же fBm, но с другим смещением, чтобы структура не
        // совпадала с полосой.
        const float nebulaNoise = skyFbm({ dir.x, dir.y, dir.z }, seed + 9091u, 5, 2.2f, 0.52f);
        const float nebulaRamp = std::pow(std::max(0.f, nebulaNoise - 0.50f) * 2.4f, 1.9f);
        const float nebulaAmount = nebulaRamp * settings.nebula * (0.35f + 0.65f * band);

        // Пыль: очень слабый крупный fBm, чтобы фон не был плоским.
        const float dust = skyFbm({ dir.x, dir.y, dir.z }, seed + 1234u, 3, 1.4f, 0.5f);

        SkyVec3 color = { 0.0040f + 0.010f * dust, 0.0048f + 0.011f * dust, 0.0105f + 0.018f * dust };

        // Млечный Путь: тёплое ядро + холодные края.
        const float galaxyAmount = band * bandTexture * settings.galaxy;
        color.x += galaxyAmount * 0.055f;
        color.y += galaxyAmount * 0.050f;
        color.z += galaxyAmount * 0.066f;

        // Туманности: фиолетово-бирюзовая палитра.
        const SkyVec3 nebulaColor = { 0.42f, 0.13f, 0.52f };
        const SkyVec3 tealColor = { 0.08f, 0.30f, 0.40f };
        const float mix = std::min(1.f, std::max(0.f, nebulaNoise * 1.6f - 0.25f));
        color.x += nebulaAmount * (nebulaColor.x * (1.f - mix) + tealColor.x * mix);
        color.y += nebulaAmount * (nebulaColor.y * (1.f - mix) + tealColor.y * mix);
        color.z += nebulaAmount * (nebulaColor.z * (1.f - mix) + tealColor.z * mix);

        // Звёзды: сферические координаты направления.
        const float theta = std::acos(std::min(1.f, std::max(-1.f, dir.y)));      // 0..pi
        const float phi = std::atan2(dir.z, dir.x);                               // -pi..pi
        const float uu = (phi + kSkyPi) / (2.f * kSkyPi);
        const float vv = theta / kSkyPi;

        // Вдоль полосы звёзд больше — так небо выглядит связно.
        const float densityScale = settings.starDensity * (1.f + 0.55f * band);

        float starGlow = 0.f;
        for (int layer = 0; layer < 3; ++layer)
            starGlow += skyStarField(uu, vv, layer, seed, densityScale, settings.faceSize);

        const SkyVec3 color0 = skyStarColor(seed);
        const float twinkleVariation = skyFbm({ dir.x * 24.f, dir.y * 24.f, dir.z * 24.f }, seed + 313u, 2, 1.f, 0.5f);
        const float stars = starGlow * settings.starBrightness * (0.75f + 0.5f * twinkleVariation);
        color.x += stars * color0.x;
        color.y += stars * color0.y;
        color.z += stars * color0.z;

        // Подсветка у горизонта: без неё небо стыкуется с туманом «ножом».
        const float horizon = std::min(1.f, std::max(0.f, (0.18f - dir.y) / 0.38f));
        const float horizonAmount = horizon * horizon * settings.horizonGlow;
        color.x += horizonAmount * 0.030f;
        color.y += horizonAmount * 0.042f;
        color.z += horizonAmount * 0.070f;

        color.x *= settings.exposure;
        color.y *= settings.exposure;
        color.z *= settings.exposure;

        color.x = std::min(1.f, std::max(0.f, color.x));
        color.y = std::min(1.f, std::max(0.f, color.y));
        color.z = std::min(1.f, std::max(0.f, color.z));
        return color;
    }

    // Стандартная раскладка кубемапы (ARB / OpenGL): 0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z.
    // Для симметричного звёздного поля порядок граней косметичен — швов не будет
    // при любом согласованном варианте, важно лишь постоянство внутри набора.
    SkyVec3 skyFaceDirection(int face, float s, float t)
    {
        switch (face)
        {
            case 0: return { 1.f, -t, -s };
            case 1: return { -1.f, -t, s };
            case 2: return { s, 1.f, t };
            case 3: return { s, -1.f, -t };
            case 4: return { s, -t, 1.f };
            default: return { -s, -t, -1.f };
        }
    }

    int skyRoundToPowerOfTwo(int value, int low, int high)
    {
        value = std::min(high, std::max(low, value));
        int result = 1;
        while (result * 2 <= value) result *= 2;
        // ближайшая степень двойки, а не «вниз»
        if (value - result > result) result *= 2;
        return result;
    }
}

namespace CubemapGenerator
{
    std::string faceFileName(int face)
    {
        return "cubemap_" + std::to_string(face) + ".png";
    }

    uint64_t estimatedBytes(const Settings& settings)
    {
        const int size = skyRoundToPowerOfTwo(settings.faceSize, 64, 2048);
        // Оценка «на глаз» по факту: звёздное поле сжимается примерно в 8 раз.
        return static_cast<uint64_t>(size) * size * 4ull * 6ull / 8ull;
    }

    bool generate(const std::filesystem::path& folder,
                  const Settings& settings,
                  const std::atomic<bool>* cancel,
                  std::string* errorOut)
    {
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        if (ec)
        {
            if (errorOut) *errorOut = "не смог создать " + folder.string() + ": " + ec.message();
            return false;
        }

        const int size = skyRoundToPowerOfTwo(settings.faceSize, 64, 2048);
        std::vector<uint8_t> pixels(static_cast<size_t>(size) * size * 4);

        // Звёзды считают размер ячейки от фактического размера грани, поэтому
        // дальше работаем с приведённым (степень двойки) значением, а не с тем,
        // что пришло из настроек.
        Settings skyNormalized = settings;
        skyNormalized.faceSize = size;

        const auto started = std::chrono::steady_clock::now();

        for (int face = 0; face < 6; ++face)
        {
            if (cancel && cancel->load())
            {
                if (errorOut) *errorOut = "генерация отменена";
                return false;
            }

            for (int y = 0; y < size; ++y)
            {
                const float t = (2.f * (static_cast<float>(y) + 0.5f) / static_cast<float>(size)) - 1.f;

                for (int x = 0; x < size; ++x)
                {
                    const float s = (2.f * (static_cast<float>(x) + 0.5f) / static_cast<float>(size)) - 1.f;

                    const SkyVec3 dir = skyNormalize(skyFaceDirection(face, s, t));
                    const SkyVec3 color = skySampleColor(dir, skyNormalized);

                    const size_t offset = (static_cast<size_t>(y) * size + x) * 4;
                    pixels[offset + 0] = static_cast<uint8_t>(color.x * 255.f + 0.5f);
                    pixels[offset + 1] = static_cast<uint8_t>(color.y * 255.f + 0.5f);
                    pixels[offset + 2] = static_cast<uint8_t>(color.z * 255.f + 0.5f);
                    pixels[offset + 3] = 255;   // небо непрозрачное
                }
            }

            const auto file = folder / faceFileName(face);
            if (!PngWriter::writeRGBA(file, size, size, pixels.data()))
            {
                if (errorOut) *errorOut = "не смог записать " + file.string();
                return false;
            }

            spdlog::info("[CubemapGenerator] грань {} готова ({})", face, file.string());
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        spdlog::info("[CubemapGenerator] шесть граней {}x{} готовы за {} мс (сид {})",
                     size, size, elapsed, settings.seed);
        return true;
    }
}
