#pragma once
//
// CubemapGenerator — процедурное космическое небо: пишет шесть граней
// `cubemap_0..5.png`, которые дальше ставит ClientPack (SkyPack).
//
// Зачем генератор, а не готовые картинки. У игрока нет ассетов, а любая чужая
// картинка — это лицензия, вес и «не то, что хотелось». Генерация даёт три вещи
// сразу: ничего не надо скачивать, размер граней и плотность звёзд настраиваются
// ползунком, и — что важнее всего для кубемапы — **нет швов**. Поле считается от
// направления (x, y, z), поэтому звезда, стоящая на ребре между гранями,
// выглядит одинаково с обеих сторон, и ориентация граней перестаёт быть
// проблемой: у симметричного звёздного поля её просто не видно.
//
// Ориентация граней — стандартная (ARB cubemap), та же, что в ванильном
// `textures/environment/overworld_cubemap`: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z.
//
// PNG пишется своим энкодером (Utils/MiscUtils/PngWriter.hpp): без zlib и libpng,
// фиксированный Huffman + LZ77. Звёздное небо сжимается в разы, а на 512²
// грани установка занимает доли секунды.
//
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>

namespace CubemapGenerator
{
    struct Settings
    {
        int faceSize = 512;            // 64..2048; округляется до степени двойки
        uint32_t seed = 20260918;      // один и тот же сид = одно и то же небо

        float starDensity = 1.0f;      // 0..3   — сколько звёзд
        float starBrightness = 1.0f;   // 0..2   — яркость звёзд
        float nebula = 0.55f;          // 0..1.5 — туманности (fBm)
        float galaxy = 0.85f;          // 0..2   — полоса Млечного Пути
        float horizonGlow = 0.35f;     // 0..1   — подсветка у «горизонта»
        float exposure = 1.0f;         // 0.2..2 — общая экспозиция
    };

    // Имя файла грани: cubemap_0.png .. cubemap_5.png
    std::string faceFileName(int face);

    // Пишет шесть граней в folder (создаёт её при необходимости).
    // cancel — если станет true, генерация прерывается и функция вернёт false:
    // модуль может выключиться, пока считается 1024².
    bool generate(const std::filesystem::path& folder,
                  const Settings& settings,
                  const std::atomic<bool>* cancel = nullptr,
                  std::string* errorOut = nullptr);

    // Размер одного файла грани в байтах (для лога/предупреждений).
    uint64_t estimatedBytes(const Settings& settings);
}
