#pragma once
#include <Features/Modules/Module.hpp>

class NameProtect : public ModuleBase<NameProtect> {
public:
    // Nick settings
    EnumSetting mTextColor = EnumSetting("Text Color", "Minecraft color code for the nick", 0,
        "White", "Black", "Gray", "Dark Gray",
        "Red", "Dark Red", "Gold", "Yellow",
        "Green", "Dark Green", "Aqua", "Dark Aqua",
        "Blue", "Dark Blue", "Light Purple", "Dark Purple"
    );

    BoolSetting mBold = BoolSetting("Bold", "Make the nick bold", false);
    BoolSetting mObfuscated = BoolSetting("Obfuscated", "Make the nick obfuscated (scrambled)", false);
    BoolSetting mItalic = BoolSetting("Italic", "Make the nick italic", true);
    BoolSetting mReplaceChatName = BoolSetting("Replace In Chat", "Replace your name in chat messages too", true);

    NameProtect() : ModuleBase("NameProtect", "Changes your name with color/style support (Nick)", ModuleCategory::Visual, 0, false) {
        mNames = {
                {Lowercase, "nameprotect"},
                {LowercaseSpaced, "name protect"},
                {Normal, "NameProtect"},
                {NormalSpaced, "Name Protect"}
        };

        addSettings(
            &mTextColor,
            &mBold,
            &mObfuscated,
            &mItalic,
            &mReplaceChatName
        );

        // god fucking dammit alteik
        mNewName = Solstice::Prefs->mStreamerName;
    }
    std::string mNewName = "";

    std::string mOldLocalName = "";
    std::string mOldNameTag = "";
    std::string mLastStyledNick = "";

    std::string getColorCode() const;
    std::string buildStyledNick() const;

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(class BaseTickEvent& event);
    void onPacketInEvent(class PacketInEvent& event);
};