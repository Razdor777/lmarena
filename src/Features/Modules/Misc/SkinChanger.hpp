#pragma once
//
// SkinChanger - Replace other players' skins with custom skin
//

#include <Features/Modules/Module.hpp>
#include <vector>
#include <string>

class SkinChanger : public ModuleBase<SkinChanger> {
public:
    // Настройки
    BoolSetting mReplaceSkins = BoolSetting("Replace Skins", "Replace other players skins", true);
    BoolSetting mReplaceSelf = BoolSetting("Replace Self", "Also replace your own skin", false);
    
    // Хранение кастомного скина (static для доступа из хука)
    static inline std::vector<uint8_t> mCustomSkinData;
    static inline int mCustomSkinWidth = 64;
    static inline int mCustomSkinHeight = 64;
    static inline bool mSkinLoaded = false;
    static inline std::string mCurrentSkinPath = "";
    
    SkinChanger() : ModuleBase("SkinChanger", "Replace other players skins with custom skin", ModuleCategory::Misc, 0, false) {
        addSettings(&mReplaceSkins, &mReplaceSelf);
        
        mNames = {
            {Lowercase, "skinchanger"},
            {LowercaseSpaced, "skin changer"},
            {Normal, "SkinChanger"},
            {NormalSpaced, "Skin Changer"},
        };
    }
    
    static bool loadSkinFromFile(const std::string& path);
    
    void onEnable() override;
    void onDisable() override;
    
    std::string getSettingDisplay() override {
        return mSkinLoaded ? "Loaded" : "No Skin";
    }
};