//
// Created by vastrakai on 6/28/2024.
//

#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "Packet.hpp"

enum class TextPacketType : unsigned char {
    Raw                    = 0x0,
    Chat                   = 0x1,
    Translate              = 0x2,
    Popup                  = 0x3,
    JukeboxPopup           = 0x4,
    Tip                    = 0x5,
    SystemMessage          = 0x6,
    Whisper                = 0x7,
    Announcement           = 0x8,
    TextObjectWhisper      = 0x9,
    TextObject             = 0xA,
    TextObjectAnnouncement = 0xB,
    null                   = 255,
};

// Layout re-pinned for 26.62.
//
// Up to 1.21.44 the packet was flat: mType / mAuthor / mMessage /
// mFilteredMessage / mLocalize / mParams ... as separate members at +0x30..+0xE0.
// 26.62 replaces the author/message part with a std::variant holding one of
// three shapes (MessageOnly, AuthorAndMessage, MessageAndParams). Reading a flat
// mMessage at the old offset now returns garbage, so the variant has to be
// unwrapped.
//
// About 70 call sites in the client do `packet->mMessage` / `packet->mAuthor`.
// To keep them all working, the message fields are exposed as *methods* with the
// same names: `packet->mMessage` resolves to mMessage() and yields a value just
// like the old member did. new code should prefer getMessage() / getSource().
class TextPacket : public Packet { // 1.21.131
public:
    static const PacketID ID = PacketID::Text;

    struct AuthorAndMessage {
        TextPacketType mType;
        std::string    mAuthor;
        std::string    mMessage;
    };

    struct MessageAndParams {
        TextPacketType           mType;
        std::string              mMessage;
        std::vector<std::string> mParams;
    };

    struct MessageOnly {
        TextPacketType mType;
        std::string    mMessage;
    };

    using Body = std::variant<MessageOnly, AuthorAndMessage, MessageAndParams>;

    bool                        mLocalize;
    std::string                 mXuid;
    std::string                 mPlatform;
    std::optional<std::string>  mFilteredMessage;
    Body                        mBody;

    // ── compatibility accessors (same spelling as the 1.21.44 members) ───────
    TextPacketType mType() const {
        switch (mBody.index()) {
        case 0: return std::get<0>(mBody).mType;
        case 1: return std::get<1>(mBody).mType;
        case 2: return std::get<2>(mBody).mType;
        default: return TextPacketType::null;
        }
    }

    std::string mMessage() const {
        switch (mBody.index()) {
        case 0: return std::get<0>(mBody).mMessage;
        case 1: return std::get<1>(mBody).mMessage;
        case 2: return std::get<2>(mBody).mMessage;
        default: return {};
        }
    }

    // Only the Chat/Whisper shape carries an author; everything else is empty,
    // which is what the old flat field contained for those types too.
    std::string mAuthor() const {
        if (mBody.index() == 1) return std::get<1>(mBody).mAuthor;
        return {};
    }

    std::vector<std::string> mParams() const {
        if (mBody.index() == 2) return std::get<2>(mBody).mParams;
        return {};
    }

    // ── canonical accessors ─────────────────────────────────────────────────
    TextPacketType getType() const { return mType(); }

    std::string getMessage() const { return mMessage(); }

    std::string getSource() const { return mAuthor(); }

    bool hasAuthor() const { return mBody.index() == 1; }

    // Fills the packet for outgoing chat, mirroring the old field assignment.
    void setChat(const std::string& author, const std::string& message) {
        mLocalize = false;
        mBody = AuthorAndMessage{ TextPacketType::Chat, author, message };
    }

    void setMessage(const std::string& message) {
        switch (mBody.index()) {
        case 0: std::get<0>(mBody).mMessage = message; break;
        case 1: std::get<1>(mBody).mMessage = message; break;
        case 2: std::get<2>(mBody).mMessage = message; break;
        default: mBody = MessageOnly{ TextPacketType::Raw, message }; break;
        }
    }
};
