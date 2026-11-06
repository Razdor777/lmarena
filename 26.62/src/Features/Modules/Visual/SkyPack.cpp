#include "SkyPack.hpp"

#include <Features/FeatureManager.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>

namespace
{
    // Сколько ГРАНЕЙ кубемапы уже лежит в папке (0..6).
    //
    // Считаем именно шесть, а не «хоть одну». Это была настоящая причина, по
    // которой небо не менялось: генератор пишет грани по одной, `skyPackHasAny-
    // Textures` срабатывал на первом же cubemap_0.png, и в пак уезжала ОДНА
    // грань из шести (на живой установке в паке лежит ровно
    // textures/environment/overworld_cubemap/cubemap_0.png, и больше ничего).
    // Остальные пять граней оставались ванильными.
    //
    // Имя с префиксом — из-за unity-сборки (CMAKE_UNITY_BUILD в CMakeLists):
    // файл склеивается с соседями в одну единицу трансляции, и слишком общее имя
    // легко совпадёт с чужим из той же склейки. Анонимное пространство имён от
    // этого не защищает — оно как раз делает имя видимым в глобальной области.
    // Регистр прощаем так же, как ClientPack: игра ищет по индексу и регистр
    // различает, а Windows — нет.
    int skyPackFaceCount(const std::filesystem::path& folder)
    {
        std::error_code ec;
        if (!std::filesystem::exists(folder, ec)) return 0;

        int faces = 0;
        for (const auto& entry : std::filesystem::directory_iterator(folder, ec))
        {
            if (!entry.is_regular_file(ec)) continue;

            std::string name = entry.path().filename().string();
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (name.rfind("cubemap_", 0) != 0 || name.size() != std::string("cubemap_0.png").size())
                continue;
            if (name.compare(name.size() - 4, 4, ".png") != 0) continue;

            const char digit = name[8];
            if (digit >= '0' && digit <= '5') ++faces;
        }

        return faces;
    }

    // Готовое дерево textures\... или свой manifest.json — тогда пак собирается не
    // из наших шести граней, и требовать шесть файлов в корне нельзя.
    bool skyPackHasTexturesTree(const std::filesystem::path& folder)
    {
        std::error_code ec;
        return std::filesystem::exists(folder / "manifest.json", ec) ||
               std::filesystem::exists(folder / "textures", ec);
    }
}

void SkyPack::ensureSourceFolderExists()
{
    std::error_code ec;
    if (mLastResolvedFolder.empty()) return;

    std::filesystem::create_directories(mLastResolvedFolder, ec);
}

void SkyPack::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &SkyPack::onBaseTickEvent>(this);

    mTickCounter = 0;
    mInstalled = false;
    mWaitingForTextures = false;
    mGenerated = false;
    mGenerationAttempted = false;   // переключил модуль — есть право на новую попытку

    tryInstall(false);
}

void SkyPack::say(const std::string& message)
{
    if (!mChatReport.mValue) return;
    ChatUtils::displayClientMessage(message);
}

void SkyPack::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &SkyPack::onBaseTickEvent>(this);

    // Сначала останавливаем генерацию: детач потока при живом модуле был бы
    // обращением к освобождённой памяти.
    joinWorker();

    // ── Пак по умолчанию остаётся ─────────────────────────────────────────
    // Смысл в том, ЧТО делает игра с этим паком: он применяется только при
    // следующем запуске игры (глобальный список) или входе в мир (список мира).
    // Если снимать пак при выключении модуля, то переключение модуля в клиенте
    // гарантированно убивает небо ещё до того, как игрок его увидит, и выглядит
    // это ровно как «Sky Pack не работает». Поэтому удаляем только по явной
    // просьбе.
    if (!mRemoveOnDisable.mValue)
    {
        mInstalled = false;
        mStatus = "left installed";

        const std::string packName = mPackName.mValue.empty() ? "Solstice Sky" : mPackName.mValue;
        spdlog::info("[SkyPack] модуль выключен, пак \"{}\" оставлен в игре", packName);
        say("§bSky Pack§r: пак оставлен в игре (Remove On Disable = выкл) — небо из него "
            "останется до тех пор, пока ты не включишь Remove On Disable и не выключишь модуль снова.");
        return;
    }

    // Удаление идемпотентно: чистим даже если модуль был включён в прошлом
    // запуске и мы не знаем, дошли ли тогда до установки.
    const std::string packName = mPackName.mValue.empty() ? "Solstice Sky" : mPackName.mValue;

    std::string message;
    const bool ok = ClientPack::uninstall(packName, mComMojangPath.mValue, &message);

    mInstalled = false;
    mStatus = ok ? "removed" : "remove failed";

    if (mNotify.mValue)
    {
        NotifyUtils::notify(ok ? ("Sky pack removed: " + packName)
                              : ("Sky pack not removed: " + message),
                            ok ? 3.f : 8.f,
                            ok ? Notification::Type::Info : Notification::Type::Error);
    }
}

void SkyPack::startGeneration()
{
    if (mGenerating.load()) return;

    // Поток мог уже отработать и просто ждать джойна — это дёшево.
    if (mWorker.joinable()) mWorker.join();

    mCancel = false;
    mGenerating = true;
    mGenerated = false;
    mWorkerError.clear();

    const std::filesystem::path folder = mLastResolvedFolder;

    CubemapGenerator::Settings settings;
    settings.faceSize = mFaceSize.mValue;
    settings.seed = static_cast<uint32_t>(mSeed.mValue);
    settings.starDensity = mStars.mValue;
    settings.starBrightness = mStarBright.mValue;
    settings.nebula = mNebula.mValue;
    settings.galaxy = mGalaxy.mValue;
    settings.horizonGlow = mGlow.mValue;
    settings.exposure = mExposure.mValue;

    mStatus = "generating cosmic sky";

    if (mNotify.mValue)
    {
        NotifyUtils::notify(
            "Sky Pack: строю звёздное небо " + std::to_string(static_cast<int>(settings.faceSize)) +
            "x" + std::to_string(static_cast<int>(settings.faceSize)) + "...",
            5.f, Notification::Type::Info);
    }

    mWorker = std::thread([this, folder, settings]()
    {
        std::string error;
        const bool ok = CubemapGenerator::generate(folder, settings, &mCancel, &error);

        mWorkerError = error;
        mGenerated = ok;
        mGenerating = false;
    });
}

void SkyPack::joinWorker()
{
    mCancel = true;
    if (mWorker.joinable()) mWorker.join();
    mGenerating = false;
}

void SkyPack::tryInstall(bool fromTick)
{
    ClientPack::InstallRequest request;
    request.packName = mPackName.mValue.empty() ? "Solstice Sky" : mPackName.mValue;
    request.sourceFolder = mSourceFolder.mValue.empty() ? ClientPack::defaultSourceFolder()
                                                        : std::filesystem::path(mSourceFolder.mValue);
    request.comMojangOverride = mComMojangPath.mValue;
    request.includeEnd = mIncludeEnd.mValue;

    mLastResolvedFolder = request.sourceFolder.string();

    // Папку создаём сразу: она нужна и генератору, и пользователю как адрес.
    ensureSourceFolderExists();

    // Генерация идёт → ждём её конца, что бы в папке уже ни лежало. Иначе мы
    // могли бы скопировать в пак прошлогодние (или только что перезаписанные)
    // файлы прямо в процессе записи.
    if (mGenerating.load())
    {
        mStatus = "generating cosmic sky";
        mInstalled = false;
        return;
    }

    // ── Готовность папки ────────────────────────────────────────────────────
    // Пак ставим только когда текстуры ЦЕЛЫЕ: либо своё дерево textures\...,
    // либо все шесть граней кубемапы. Ни одной, ни пяти — мало.
    const bool hasTree = skyPackHasTexturesTree(request.sourceFolder);
    const int  faces   = skyPackFaceCount(request.sourceFolder);
    const bool hasTextures = hasTree || (faces == 6);

    // ── Космос: если своих текстур нет, рисуем небо сами ─────────────────────
    if (!hasTextures && mCosmic.mValue)
    {
        // Попытка ровно одна на включение модуля. Без этого флага сбой
        // генерации (например, папка только для чтения) перезапускал бы её
        // каждую секунду — вечный цикл, который видно только по нагрузке.
        if (!mGenerationAttempted)
        {
            mGenerationAttempted = true;
            startGeneration();
            mStatus = "generating cosmic sky";
            mInstalled = false;
            return;
        }

        mStatus = faces > 0 ? "cosmic sky incomplete" : "cosmic generation failed";
        mInstalled = false;

        if (!fromTick)
        {
            spdlog::error("[SkyPack] генерация неба не удалась (граней: {}/6): {}",
                          faces, mWorkerError);
            if (mNotify.mValue)
            {
                NotifyUtils::notify("Sky Pack: не смог построить небо — " + mWorkerError +
                                        "\nИсправь и переключи модуль заново.",
                                    10.f, Notification::Type::Error);
            }
        }
        return;
    }

    // ── Папка есть, но она пустая и космос выключен ──────────────────────────
    if (!hasTextures && !mCosmic.mValue)
    {
        const bool firstTime = !mWaitingForTextures;
        mWaitingForTextures = true;
        mStatus = "waiting for textures";
        mInstalled = false;

        if (!fromTick && firstTime)
        {
            spdlog::info("[SkyPack] в {} пока нет текстур", mLastResolvedFolder);
            if (mNotify.mValue)
            {
                NotifyUtils::notify(
                    "Sky Pack: включи Cosmic Sky или положи ВСЕ шесть граней "
                    "cubemap_0..5.png сюда:\n" +
                    mLastResolvedFolder,
                    12.f, Notification::Type::Info);
            }
        }
        return;
    }

    if (!std::filesystem::exists(request.sourceFolder))
    {
        mStatus = "source folder missing";
        mInstalled = false;
        return;
    }

    // ── Ставим ───────────────────────────────────────────────────────────────
    const auto result = ClientPack::install(request);

    if (!result.ok)
    {
        mInstalled = false;
        mStatus = "install failed";
        mWaitingForTextures = true;   // текстуры есть — попробуем ещё на следующем тике

        // Ошибка установки повторяется каждый тик, поэтому говорим о ней
        // только когда её вызвал сам пользователь (переключение модуля).
        if (!fromTick)
        {
            spdlog::error("[SkyPack] установка не удалась: {}", result.message);
            if (mNotify.mValue)
                NotifyUtils::notify("Sky Pack: " + result.message, 10.f, Notification::Type::Error);
        }
        return;
    }

    const bool wasInstalled = mInstalled;
    mInstalled = true;
    mWaitingForTextures = false;
    mStatus = "installed";

    if (wasInstalled) return;

    // Опции графики правим ровно в момент установки пака: без fancy skies
    // кубемапа не рисуется вовсе, и пака как будто нет.
    ClientPack::OptionsReport options;
    if (mFixGraphics.mValue)
        options = ClientPack::ensureSkyOptions(request.comMojangOverride, true, mDisableRayTracing.mValue);

    spdlog::info("[SkyPack] пак установлен: {} (файлов {}, граней {}/6, глоб.: {}, миров: {})",
                 result.packPath.string(), result.filesCopied, faces,
                 result.registeredGlobal ? "да" : "нет", result.worldCount);

    reportInstall(result, faces, options);
}

void SkyPack::reportInstall(const ClientPack::InstallResult& result, int faces,
                            const ClientPack::OptionsReport& options)
{
    const bool rtxOn = options.ok && options.rayTracingOn();
    const bool rtxFatal = rtxOn && !options.rayTracingChanged;
    const bool skiesWereOff = options.ok && !options.fancySkiesOld.empty() && !options.fancySkiesOn();

    if (mNotify.mValue)
    {
        char copiedInfo[256] = {};
        snprintf(copiedInfo, sizeof(copiedInfo), "\nграней кубемапы: %d/6, миров: %d", faces, result.worldCount);

        NotifyUtils::notify(std::string("Sky Pack установлен") +
                                (result.registeredGlobal ? "" : " (включи его вручную)") + copiedInfo +
                                "\nПолностью перезапусти игру.",
                            8.f, Notification::Type::Info);
    }

    if (!mChatReport.mValue) return;

    std::string report = "§bSky Pack§r установлен.\n";
    report += "§7папка:§r " + result.packPath.string() + "\n";
    report += fmt::format("§7файлов:§r {}, §7граней кубемапы:§r {}/6\n", result.filesCopied, faces);
    report += fmt::format("§7списки паков:§r global {}, миров {}\n",
                          result.registeredGlobal ? "§aда§r" : "§cнет§r", result.worldCount);

    if (options.ok)
    {
        report += fmt::format("§7options.txt:§r fancy skies {} §7| fancy graphics {} §7| raytracing {}\n",
                              options.fancySkiesOld.empty() ? "нет ключа" : options.fancySkiesOld,
                              options.fancyGraphicsOld.empty() ? "нет ключа" : options.fancyGraphicsOld,
                              options.rayTracingOld.empty() ? "нет ключа" : options.rayTracingOld);

        report += std::string("§7изменилось:§r ") +
                  (options.fancySkiesChanged ? "Fancy Skies → 1 " : "") +
                  (options.fancyGraphicsChanged ? "Fancy Graphics → 1 " : "") +
                  (options.rayTracingChanged ? "Ray Tracing → 0 " : "") +
                  (options.changed() ? "\n" : "ничего (уже так)\n");

        if (skiesWereOff && !options.fancySkiesChanged && !options.wrote)
            report += "§eFancy Skies всё ещё 0§r — включи в Настройки → Видео, иначе небо не появится.\n";
    }
    else
    {
        report += "§eoptions.txt не проверен:§r " + options.message + "\n";
    }

    if (rtxFatal)
        report += "§cВключён Ray Tracing§r — небо рисует он сам, кубемапа из пака не используется. "
                  "Выключи Ray Tracing (или включи настройку Disable Ray Tracing) — без этого небо не поменяется.\n";

    report += "§7что дальше:§r полностью закрой и открой игру. Если небо не появилось — включи "
              "§fFancy Graphics и Fancy Skies§r в Настройки → Видео (игра могла перезаписать options.txt при выходе). "
              "Сам пак должен быть включён в §fНастройки → Глобальные ресурсы§r.\n";

    say(report);
}

void SkyPack::onBaseTickEvent(BaseTickEvent& event)
{
    (void)event;

    // Проверяем раз в секунду (20 тиков), пока пак не встал. Так модуль не
    // заставляет пользователя перезапускать клиент после того, как он закинул
    // картинки, и при этом ничего не делает, когда уже всё готово.
    if (++mTickCounter < 20) return;
    mTickCounter = 0;

    if (mInstalled) return;
    tryInstall(true);
}
