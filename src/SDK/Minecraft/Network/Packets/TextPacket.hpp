//
// Created by vastrakai on 6/28/2024.
//

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include "Packet.hpp"
#include <SDK/Minecraft/MinecraftGame.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

enum class TextPacketType : unsigned char {
    Raw = 0x0,
    Chat = 0x1,
    Translate = 0x2,
    Popup = 0x3,
    JukeboxPopup = 0x4,
    Tip = 0x5,
    SystemMessage = 0x6,
    Whisper = 0x7,
    Announcement = 0x8,
    TextObjectWhisper = 0x9,
    TextObject = 0xA,
    TextObjectAnnouncement = 0xB,
    null = 255
};

class TextPacket : public Packet { // 1.21.131+ / 26.62
public:
    static const PacketID ID = PacketID::Text;

    struct AuthorAndMessage {
        TextPacketType mType;
        std::string mAuthor;
        std::string mMessage;
    };

    struct MessageAndParams {
        TextPacketType mType;
        std::string mMessage;
        std::vector<std::string> mParams;
    };

    struct MessageOnly {
        TextPacketType mType;
        std::string mMessage;
    };

    bool mLocalize = false;
    std::string mXuid;
    std::string mPlatform;
    std::optional<std::string> mFilteredMessage;
    std::variant<MessageOnly, AuthorAndMessage, MessageAndParams> mBody;

    void Messeage(Actor* player, std::string msgs) {
        mLocalize = false;
        auto* ci = ClientInstance::get();
        auto* mc = ci ? ci->getMinecraftGame() : nullptr;
        mXuid = mc ? mc->getxuid() : std::string{};
        mBody = AuthorAndMessage{ TextPacketType::Chat, player ? player->getLocalName() : std::string{}, std::move(msgs) };
    }

    TextPacketType gettype() const {
        switch (mBody.index()) {
        case 0: return std::get<0>(mBody).mType;
        case 1: return std::get<1>(mBody).mType;
        case 2: return std::get<2>(mBody).mType;
        default: return TextPacketType::null;
        }
    }

    TextPacketType getType() const {
        return gettype();
    }

    void setType(TextPacketType type) {
        const std::string curMsg = getMessage();
        const std::string curAuthor = getSource();
        const bool wantAuthor = (type == TextPacketType::Chat || type == TextPacketType::Whisper || type == TextPacketType::Announcement);
        const bool wantParams = (type == TextPacketType::Translate || type == TextPacketType::Popup || type == TextPacketType::JukeboxPopup);

        if (wantAuthor && mBody.index() != 1) {
            mBody = AuthorAndMessage{ type, curAuthor, curMsg };
            return;
        }
        if (wantParams && mBody.index() != 2) {
            mBody = MessageAndParams{ type, curMsg, {} };
            return;
        }

        switch (mBody.index()) {
        case 0: std::get<0>(mBody).mType = type; break;
        case 1: std::get<1>(mBody).mType = type; break;
        case 2: std::get<2>(mBody).mType = type; break;
        default: mBody = MessageOnly{ type, curMsg }; break;
        }
    }

    std::string& messageRef() {
        static std::string sDummyMsg;
        switch (mBody.index()) {
        case 0: return std::get<0>(mBody).mMessage;
        case 1: return std::get<1>(mBody).mMessage;
        case 2: return std::get<2>(mBody).mMessage;
        default: sDummyMsg.clear(); return sDummyMsg;
        }
    }

    std::string getMessage() const {
        switch (mBody.index()) {
        case 0: return std::get<0>(mBody).mMessage;
        case 1: return std::get<1>(mBody).mMessage;
        case 2: return std::get<2>(mBody).mMessage;
        default: return "";
        }
    }

    void setMessage(const std::string& v) {
        messageRef() = v;
    }

    std::string& authorRef() {
        if (mBody.index() != 1) {
            mBody = AuthorAndMessage{ gettype(), std::string{}, getMessage() };
        }
        return std::get<1>(mBody).mAuthor;
    }

    const std::string& getAuthorRef() const {
        static const std::string sEmpty;
        if (mBody.index() == 1) {
            return std::get<1>(mBody).mAuthor;
        }
        return sEmpty;
    }

    std::string getSource() const {
        return getAuthorRef();
    }

    void setAuthor(const std::string& v) {
        authorRef() = v;
    }

    const std::vector<std::string>& getParamsRef() const {
        static const std::vector<std::string> sEmptyParams;
        if (mBody.index() == 2) {
            return std::get<2>(mBody).mParams;
        }
        return sEmptyParams;
    }

    void setParams(const std::vector<std::string>& params) {
        if (mBody.index() != 2) {
            mBody = MessageAndParams{ gettype(), getMessage(), params };
        } else {
            std::get<2>(mBody).mParams = params;
        }
    }

    // Zero-size MSVC/clang-cl property aliases for legacy field access
    __declspec(property(get = getType, put = setType)) TextPacketType mType;
    __declspec(property(get = messageRef, put = setMessage)) std::string mMessage;
    __declspec(property(get = getAuthorRef, put = setAuthor)) std::string mAuthor;
    __declspec(property(get = getAuthorRef, put = setAuthor)) std::string mSource;
    __declspec(property(get = getParamsRef, put = setParams)) std::vector<std::string> mParams;
};
