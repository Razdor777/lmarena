//
// Created by alteik on 12/10/2024.
// Extended with Flarial-style Nick features
//

#include "NameProtect.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Network/Packets/Packet.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <regex>

// Map color names to Minecraft §-codes
static const std::map<std::string, std::string> sColorCodes = {
    {"White", "\xC2\xA7" "f"},
    {"Black", "\xC2\xA7" "0"},
    {"Gray", "\xC2\xA7" "7"},
    {"Dark Gray", "\xC2\xA7" "8"},
    {"Red", "\xC2\xA7" "c"},
    {"Dark Red", "\xC2\xA7" "4"},
    {"Gold", "\xC2\xA7" "6"},
    {"Yellow", "\xC2\xA7" "e"},
    {"Green", "\xC2\xA7" "a"},
    {"Dark Green", "\xC2\xA7" "2"},
    {"Aqua", "\xC2\xA7" "b"},
    {"Dark Aqua", "\xC2\xA7" "3"},
    {"Blue", "\xC2\xA7" "9"},
    {"Dark Blue", "\xC2\xA7" "1"},
    {"Light Purple", "\xC2\xA7" "d"},
    {"Dark Purple", "\xC2\xA7" "5"},
};

std::string NameProtect::getColorCode() const {
    std::string colorName = mTextColor.mValues[mTextColor.mValue];
    auto it = sColorCodes.find(colorName);
    if (it != sColorCodes.end()) return it->second;
    return "\xC2\xA7" "f"; // default white
}

std::string NameProtect::buildStyledNick() const {
    std::string prefix = getColorCode();

    if (mItalic.mValue)     prefix += "\xC2\xA7" "o";
    if (mBold.mValue)       prefix += "\xC2\xA7" "l";
    if (mObfuscated.mValue) prefix += "\xC2\xA7" "k";

    return prefix + mNewName + "\xC2\xA7" "r";
}

void NameProtect::onEnable() {
    mOldLocalName = "";
    mOldNameTag = "";
    mLastStyledNick = "";

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &NameProtect::onBaseTickEvent, nes::event_priority::ABSOLUTE_FIRST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &NameProtect::onPacketInEvent, nes::event_priority::ABSOLUTE_FIRST>(this);
}

void NameProtect::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &NameProtect::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &NameProtect::onPacketInEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player && !mOldLocalName.empty()) {
        player->setLocalName(mOldLocalName);
        player->setNametag(mOldNameTag);
    }
    mOldLocalName = "";
    mOldNameTag = "";
    mLastStyledNick = "";
}

static std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    if (from.empty() || to.empty()) return str;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

void NameProtect::onBaseTickEvent(BaseTickEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    std::string localName = player->getLocalName();
    std::string lowerName = localName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    // Capture the name only if it's a valid username (not empty, not "player", and not our spoofed name)
    if (mOldLocalName.empty() && !localName.empty() && lowerName != "player" && localName != mNewName) {
        mOldLocalName = localName;
        mOldNameTag = player->getNameTag();
    }

    if (mOldLocalName.empty()) return;

    std::string currentNametag = player->getNameTag();
    std::string styledNick = buildStyledNick();

    // Case 1: Nametag contains our real old username (e.g. "[Player] harigato_kyzaima")
    if (currentNametag.find(mOldLocalName) != std::string::npos) {
        std::string newNametag = replaceAll(currentNametag, mOldLocalName, styledNick);
        player->setNametag(newNametag);
    }
    // Case 2: Nametag already contains mLastStyledNick, but settings changed (e.g. bold toggled)
    else if (!mLastStyledNick.empty() && currentNametag.find(mLastStyledNick) != std::string::npos) {
        if (mLastStyledNick != styledNick) {
            std::string newNametag = replaceAll(currentNametag, mLastStyledNick, styledNick);
            player->setNametag(newNametag);
        }
    }

    mLastStyledNick = styledNick;

    // Set local name (pause menu, etc.) — plain name without formatting
    player->setLocalName(mNewName);
}

void NameProtect::onPacketInEvent(PacketInEvent& event) {
    if (!mReplaceChatName.mValue) return;
    if (event.mPacket->getId() != PacketID::Text) return;

    auto packet = event.getPacket<TextPacket>();
    if (!packet) return;

    // Only modify actual chat / whisper / announcement / raw messages — avoid system/popup packets
    switch (packet->mType) {
        case TextPacketType::Raw:
        case TextPacketType::Chat:
        case TextPacketType::Whisper:
        case TextPacketType::Announcement:
        case TextPacketType::TextObjectWhisper:
        case TextPacketType::TextObject:
        case TextPacketType::TextObjectAnnouncement:
            break;
        default:
            return; // don't touch system / popup / tip packets
    }

    if (mOldLocalName.empty()) {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (player) {
            std::string localName = player->getLocalName();
            std::string lowerName = localName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (!localName.empty() && lowerName != "player" && localName != mNewName) {
                mOldLocalName = localName;
                mOldNameTag = player->getNameTag();
            }
        }
    }
    if (mOldLocalName.empty()) return;

    std::string styledNick = buildStyledNick();
    if (styledNick.empty() || styledNick == mOldLocalName) return;

    // Replace real name with styled nick in safe client-side fields only.
    // mFilteredMessage and mParams are BDS/server-only fields — accessing them
    // on the client causes a memory layout mismatch and instant crash.
    if (!packet->mMessage.empty() && packet->mMessage.find(mOldLocalName) != std::string::npos) {
        packet->mMessage = replaceAll(packet->mMessage, mOldLocalName, styledNick);
    }
    if (!packet->mAuthor.empty() && packet->mAuthor.find(mOldLocalName) != std::string::npos) {
        packet->mAuthor = replaceAll(packet->mAuthor, mOldLocalName, styledNick);
    }
}
