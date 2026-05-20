#pragma once
//
// Created by vastrakai on 8/24/2024.
// Fixed for Render.com deployment
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/Network/Packets/TextPacket.hpp>
#include <winrt/base.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.networking.sockets.h>
#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/windows.storage.streams.h>
#include "WorkingVM.hpp"

class EncryptedOp;

enum class OpCode
{
    /* Authentication OpCodes */
    Work = 0,
    CompleteWork = 1,
    KeyIn = 2,
    KeyOut = 3,
    AuthFinish = 4,

    /* Normal OpCodes */
    IdentifyClient = 5,
    IdentifyPlayer = 6,
    ServerMessage = 7,
    Error = 8,
    Ping = 9,
    Announcement = 10,
    Join = 11,
    Leave = 12,
    Message = 13,
    ListUsers = 14,
    ConnectedUserList = 15,
    IdentifySkinData = 16,

    /* Specialized OpCodes */
    Eject = 0x8466,
    DeleteMod = 0x5836,
    ExecCommand = 0x5837,
};

class EncryptedOp {
public:
    std::string Encrypted;
    bool decrypted = false;

    EncryptedOp(std::string toEncrypt, std::string key) {
        Encrypted = StringUtils::encrypt(toEncrypt, key);
        decrypted = false;
    }

    EncryptedOp(std::string encrypted) {
        Encrypted = encrypted;
    }

    void decrypt(const std::string& key) {
        if (decrypted) return;

        if (nlohmann::json::accept(Encrypted)) {
            nlohmann::json j = nlohmann::json::parse(Encrypted);
            for (auto& [k, value] : j.items()) {
                Encrypted = value.get<std::string>();
                break;
            }
        }
        Encrypted = StringUtils::decrypt(Encrypted, key);
        decrypted = true;
    }

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["e"] = Encrypted;
        return j;
    }
};

class ChatOp {
public:
    OpCode opCode;
    std::string data;
    bool success;

    ChatOp(OpCode opCode, std::string data, bool success) 
        : opCode(opCode), data(std::move(data)), success(success) {}

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["o"] = static_cast<int>(opCode);
        j["d"] = data;
        j["s"] = success;
        return j;
    }

    static ChatOp deserialize(nlohmann::json& j) {
        OpCode opCode = static_cast<OpCode>(j["o"].get<int>());
        std::string data = j["d"].get<std::string>();
        bool success = j["s"].get<bool>();
        return ChatOp(opCode, data, success);
    }

    static ChatOp deserializeStr(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return deserialize(j);
    }

    static std::string serializeString(const ChatOp& chatOp) {
        return chatOp.serialize().dump();
    }

    ChatOp(EncryptedOp& encryptedOp, const std::string& key) {
        if (!encryptedOp.decrypted) {
            encryptedOp.decrypt(key);
        }
        nlohmann::json j = nlohmann::json::parse(encryptedOp.Encrypted);
        ChatOp chatOp = deserialize(j);
        opCode = chatOp.opCode;
        data = chatOp.data;
        success = chatOp.success;
    }
};

struct ConnectedIrcUser {
    std::string clientName;
    std::string username;
    std::string xuid;
    std::string playerName;

    ConnectedIrcUser(const std::string& clientName, const std::string& username, 
                     const std::string& xuid, const std::string& playerName) 
        : clientName(clientName), username(username), xuid(xuid), playerName(playerName) {}

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["0"] = clientName;
        j["1"] = username;
        j["2"] = playerName;
        j["3"] = xuid;
        return j;
    }

    void deserialize(const nlohmann::json& j) {
        clientName = j["0"].get<std::string>();
        username = j["1"].get<std::string>();
        playerName = j["2"].get<std::string>();
        xuid = j["3"].get<std::string>();
    }
};

namespace Sockets = winrt::Windows::Networking::Sockets;
namespace Streams = winrt::Windows::Storage::Streams;

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
};

class IrcClient {
public:
    // ===== ИЗМЕНИ ЭТО НА СВОЙ СЕРВЕР =====
    static constexpr const char* mServer = "127.0.0.1:8765";
    static constexpr bool mUseSSL = false;  // Render.com использует HTTPS
    // =====================================

    Sockets::MessageWebSocket mSocket = nullptr;
    Streams::DataWriter mWriter = nullptr;
    char mBuffer[1024];

    std::thread mReceiveThread;
    std::string mCurrentUsername = "";
    std::vector<std::string> mQueuedMessages;
    uint64_t mLastPing = 0;
    std::mutex mMutex;
    ConnectionState mConnectionState = ConnectionState::Disconnected;
    bool mIdentifyNeeded = true;

    std::mutex mConnectedUsersMutex;
    std::vector<ConnectedIrcUser> mConnectedUsers;

    bool mShowNamesInChat = false;

    bool mEncrypted = false;
    std::string mServerKey = "";
    std::string mClientKey = "";

    std::string mOldPreferredUsername = "";
    std::string mOldLocalName = "";
    std::string mOldXuid = "";

    bool mReceivedPOF = false;
    bool mAlwaysSendToIrc = false;

    std::vector<ConnectedIrcUser> getConnectedUsers();
    void setConnectedUsers(const std::vector<ConnectedIrcUser>& users);
    void sendMessage(const std::string& string);
    void listUsers();
    void changeUsername();

    IrcClient();
    ~IrcClient();
    bool isConnected() const;

    void sendOpAuto(const ChatOp& op);
    ChatOp parseOpAuto(std::string data);
    void sendData(std::string data);
    std::string getHwid();
    void genClientKey();
    void sendSkin();

    void onBaseTickEvent(BaseTickEvent& event);
    void onPacketInEvent(PacketInEvent& event);
    std::string getPreferredUsername();
    void sendPlayerIdentity(bool forced = false);
    void onPacketOutEvent(PacketOutEvent& event);

    void displayMsg(std::string message);

    bool connectToServer();
    void onConnected();
    void onReceiveOp(const ChatOp& op);
    void disconnect(std::string reason);
};

class IrcManager {
public:
    static inline std::unique_ptr<IrcClient> mClient = nullptr;
    static inline uint64_t mLastConnectAttempt = 0;

    static bool setShowNamesInChat(bool showNamesInChat);
    static bool setAlwaysSendToIrc(bool alwaysSendToIrc);
    static void init();
    static void deinit();
    static void disconnectCallback();
    static void requestListUsers();
    static void requestChangeUsername(std::string username);
    static void sendMessage(std::string& message);
    static bool isConnected();
};