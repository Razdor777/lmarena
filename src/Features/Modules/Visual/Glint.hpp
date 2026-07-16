#pragma once
#include <Features/Modules/Module.hpp>
#include <glm/glm.hpp>
#include <Hook/Detour.hpp>

class Glint : public ModuleBase<Glint> {
public:
    Glint();

    static inline Glint* instance = nullptr;
    static inline std::unique_ptr<Detour> mDetour = nullptr;

    // Настройки — используем реальные классы из Setting.hpp
    BoolSetting mRainbow   = BoolSetting("Rainbow", "Rainbow glint color", false);
    ColorSetting mColor    = ColorSetting("Color", "Glint color", 1.0f, 0.0f, 0.0f, 1.0f);
    NumberSetting mAlpha   = NumberSetting("Alpha", "Glint alpha", 1.0f, 0.0f, 1.0f, 0.01f);

    using GlintGetFunc = __int64(__fastcall*)(__int64 a1, __int64 a2, void* a3);
    static __int64 __fastcall onGetGlintComponent(__int64 a1, __int64 a2, void* a3);

    void onEnable() override;
    void onDisable() override;
};