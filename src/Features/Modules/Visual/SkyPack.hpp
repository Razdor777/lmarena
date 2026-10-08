#pragma once
//
// SkyPack — кастомное небо БЕЗ детуров, через ресурс-пак.
//
// Почему так. В 1.21.44 грани кубемапы, облака, звёзды, скайплейн и end_sky
// игра резолвит как обычные ресурсы: загрузчик строит путь
// `textures/environment/<имя>_cubemap/cubemap_<i>` и спрашивает паки, есть ли
// такой путь (RE_NOTES.md §2.2-2.3). Значит достаточно положить свой пак с
// теми же путями — и небо меняется без единого адреса и без привязки к версии.
//
// Семантика модуля простая и без скрытых кнопок:
//   * включил модуль  → пак установлен (включил в игру зайдёшь — увидишь небо);
//   * выключил модуль → пак удалён.
// Если текстуры не готовы, модуль создаёт папку с ними и ждёт: как только в
// папке появились файлы, он установит пак сам, перезапускать клиент не нужно
// (игру — нужно, потому что список паков игра пишет при выходе).
//
#include <Features/Modules/Module.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Utils/MiscUtils/ClientPack.hpp>
#include <Utils/MiscUtils/CubemapGenerator.hpp>

#include <atomic>
#include <thread>

class SkyPack : public ModuleBase<SkyPack>
{
public:
    StringSetting mPackName = StringSetting(
        "Pack Name", "Имя пака в списке ресурсов игры", "Solstice Sky");

    StringSetting mSourceFolder = StringSetting(
        "Textures Folder", "Папка с твоими текстурами неба", ClientPack::defaultSourceFolder().string());

    StringSetting mComMojangPath = StringSetting(
        "com.mojang Path", "Пусто = найти автоматически; иначе впиши путь до com.mojang", "");

    // По умолчанию только overworld: nether_cubemap игра не поддерживает (в
    // ванильном паке его нет), а подменять небо Края — вкусовое решение, поэтому
    // выключено, чтобы «космос» не появлялся там, где его не ждут.
    BoolSetting mIncludeEnd = BoolSetting(
        "End Sky Too", "Ставить кубемапу ещё и в Край (nether_cubemap игра не поддерживает)", false);

    BoolSetting mNotify = BoolSetting(
        "Notify", "Показывать уведомления о результате установки", true);

    // ── Настройки игры, без которых небо не появится ──────────────────────
    // Самая частая причина «пак поставил, а небо не поменялось» — не пак, а
    // графика игры (см. ClientPack::OptionsReport): при gfx_fancyskies:0 небо
    // рисуется плоским цветом, и кубемапа из пака вообще не читается. Поэтому
    // эти ключи проверяются и правятся вместе с установкой пака.
    BoolSetting mFixGraphics = BoolSetting(
        "Fix Graphics Options",
        "Включить Fancy Skies/Fancy Graphics в options.txt — без них кубемапа не рисуется вовсе",
        true);

    BoolSetting mDisableRayTracing = BoolSetting(
        "Disable Ray Tracing",
        "Погасить gfx_raytracing: рейтрейсинг/deferred строит небо сам, кубемапа при этом не используется",
        false);

    BoolSetting mChatReport = BoolSetting(
        "Chat Report",
        "Писать в чат, что установлено, что найдено в options.txt и что осталось сделать",
        true);

    BoolSetting mRemoveOnDisable = BoolSetting(
        "Remove On Disable",
        "Снимать пак при выключении модуля (по умолчанию пак остаётся: он применяется при следующем запуске игры)",
        false);

    // ── Космос ───────────────────────────────────────────────────────────
    // Если своих текстур нет, небо рисуется процедурно: шесть граней кубемапы
    // со звёздами, Млечным Путём и туманностями. Ничего скачивать не надо, а швов
    // между гранями не будет — поле считается от направления (см. CubemapGenerator).
    BoolSetting mCosmic = BoolSetting(
        "Cosmic Sky", "Построить звёздное небо самому, если своих текстур нет", true);
    NumberSetting mFaceSize = NumberSetting(
        "Face Size", "Размер грани кубемапы (влияет на время генерации)", 512.f, 256.f, 2048.f, 256.f);
    NumberSetting mSeed = NumberSetting(
        "Seed", "Сид генерации: одно и то же число — одно и то же небо", 20260918.f, 0.f, 999999.f, 1.f);
    NumberSetting mStars = NumberSetting(
        "Stars", "Плотность звёзд", 1.f, 0.f, 3.f, 0.1f);
    NumberSetting mStarBright = NumberSetting(
        "Star Brightness", "Яркость звёзд", 1.f, 0.f, 2.f, 0.05f);
    NumberSetting mNebula = NumberSetting(
        "Nebula", "Туманности", 0.55f, 0.f, 1.5f, 0.05f);
    NumberSetting mGalaxy = NumberSetting(
        "Galaxy", "Яркость полосы Млечного Пути", 0.85f, 0.f, 2.f, 0.05f);
    NumberSetting mGlow = NumberSetting(
        "Horizon Glow", "Подсветка у горизонта (для стыковки с туманом)", 0.35f, 0.f, 1.f, 0.05f);
    NumberSetting mExposure = NumberSetting(
        "Exposure", "Общая яркость неба", 1.f, 0.2f, 2.f, 0.05f);

    SkyPack() : ModuleBase("Sky Pack", "Custom sky via a Bedrock resource pack — no memory hooks",
                           ModuleCategory::Visual, 0, false)
    {
        mNames = {
            { Lowercase, "skypack" },
            { LowercaseSpaced, "sky pack" },
            { Normal, "SkyPack" },
            { NormalSpaced, "Sky Pack" }
        };

        addSettings(&mPackName, &mSourceFolder, &mComMojangPath, &mIncludeEnd, &mNotify,
                    &mFixGraphics, &mDisableRayTracing, &mChatReport, &mRemoveOnDisable,
                    &mCosmic, &mFaceSize, &mSeed, &mStars, &mStarBright, &mNebula, &mGalaxy,
                    &mGlow, &mExposure);

        VISIBILITY_CONDITION(mFaceSize, mCosmic.mValue);
        VISIBILITY_CONDITION(mSeed, mCosmic.mValue);
        VISIBILITY_CONDITION(mStars, mCosmic.mValue);
        VISIBILITY_CONDITION(mStarBright, mCosmic.mValue);
        VISIBILITY_CONDITION(mNebula, mCosmic.mValue);
        VISIBILITY_CONDITION(mGalaxy, mCosmic.mValue);
        VISIBILITY_CONDITION(mGlow, mCosmic.mValue);
        VISIBILITY_CONDITION(mExposure, mCosmic.mValue);

        mVisibleInArrayList.mValue = false;
    }

    // Генерация идёт в отдельном потоке: 512x512 — это несколько секунд, и
    // вешать их на игровой поток нельзя. Деструктор обязательно джойнит поток,
    // иначе std::thread уронит процесс при выгрузке клиента.
    //
    // Без `override`: у Module нет виртуального деструктора, и это здесь не
    // нужно — модули лежат в shared_ptr<Module>, а долговечность стирается в
    // делитере, так что ~SkyPack() всё равно вызовется.
    ~SkyPack() { joinWorker(); }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);

    std::string getSettingDisplay() override { return mStatus; }

    // Путь, который модуль реально использовал в последний раз (для уведомлений)
    std::string lastResolvedFolder() const { return mLastResolvedFolder; }

private:
    std::string mStatus = "idle";
    std::string mLastResolvedFolder;
    bool mInstalled = false;
    bool mWaitingForTextures = false;
    int mTickCounter = 0;

    std::thread mWorker;
    std::atomic<bool> mCancel{ false };
    std::atomic<bool> mGenerating{ false };
    std::atomic<bool> mGenerated{ false };
    // Только игровой поток: генерация пробуется один раз на включение модуля.
    bool mGenerationAttempted = false;
    std::string mWorkerError;

    void tryInstall(bool fromTick);
    void ensureSourceFolderExists();
    void startGeneration();
    void joinWorker();

    // Отчёт после установки: что положили, что прописали и что осталось сделать
    // игроку (обычно — полностью перезапустить игру).
    void reportInstall(const ClientPack::InstallResult& result, int faces,
                       const ClientPack::OptionsReport& options);
    void say(const std::string& message);
};
