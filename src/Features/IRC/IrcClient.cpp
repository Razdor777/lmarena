//
// Created by vastrakai on 8/24/2024.
// Fixed for Render.com deployment
//

#include "IrcClient.hpp"

#include <codecvt>
#include <regex>
#include <utility>
#include <Features/Command/Commands/BuildInfoCommand.hpp>
#include <Features/Events/ChatEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>
#include <Utils/OAuthUtils.hpp>

#ifdef __DEBUG__
#define logm(...) spdlog::info("[irc] " __VA_ARGS__)
#else
#define logm(...)
#endif

std::vector<ConnectedIrcUser> IrcClient::getConnectedUsers()
{
    std::lock_guard<std::mutex> lock(mConnectedUsersMutex);
    return mConnectedUsers;
}

void IrcClient::setConnectedUsers(const std::vector<ConnectedIrcUser>& users)
{
    std::lock_guard<std::mutex> lock(mConnectedUsersMutex);
    mConnectedUsers = users;
    logm("Updated connected users list, size: {}", mConnectedUsers.size());
}

void IrcClient::sendMessage(const std::string& string)
{
    auto op = ChatOp(OpCode::Message, string, true);
    sendOpAuto(op);
}

void IrcClient::listUsers()
{
    auto op = ChatOp(OpCode::ListUsers, "", true);
    sendOpAuto(op);
    logm("Requested user list");
}

void IrcClient::changeUsername()
{
    sendPlayerIdentity(true);
}

IrcClient::IrcClient()
{
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &IrcClient::onPacketOutEvent, nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &IrcClient::onBaseTickEvent, nes::event_priority::VERY_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &IrcClient::onPacketInEvent, nes::event_priority::VERY_LAST>(this);
}

IrcClient::~IrcClient()
{
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &IrcClient::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &IrcClient::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &IrcClient::onPacketInEvent>(this);
}

bool IrcClient::isConnected() const
{
    return mConnectionState == ConnectionState::Connected;
}

void IrcClient::sendOpAuto(const ChatOp& op)
{
    // Без шифрования - просто encode
    std::string serialized = op.serialize().dump();
    sendData(serialized);
}

ChatOp IrcClient::parseOpAuto(std::string data)
{
    // Данные уже декодированы в обработчике
    return ChatOp::deserializeStr(data);
}

void IrcClient::sendData(std::string data)
{
    if (!isConnected())
    {
        logm("Cannot send data, not connected to server");
        return;
    }

    try
    {
        // Кодируем в base64
        data = StringUtils::encode(data);

        std::lock_guard<std::mutex> guard(mMutex);
        mWriter.WriteString(winrt::to_hstring(data));
        mWriter.StoreAsync();
        mWriter.FlushAsync();
        
        logm("Sent data: {}", data.substr(0, 50));
    }
    catch (winrt::hresult_error const& ex)
    {
        logm("Error: {} [Code: {}]", winrt::to_string(ex.message()), ex.code());
    }
    catch (const std::exception& ex)
    {
        logm("Error: {}", ex.what());
    }
    catch (...)
    {
        logm("Unknown error sending data");
    }
}

std::string IrcClient::getHwid()
{
    return Solstice::sHWID;
}

void IrcClient::genClientKey()
{
    mClientKey = StringUtils::sha256(getHwid());
    mClientKey = mClientKey.substr(0, 16);
}

void IrcClient::sendSkin()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto skin = player->getSkin();
    int skinSize = skin->skinHeight;
    int skinDataSize = skin->skinHeight * skin->skinWidth * 4;
    std::vector<uint8_t> skinData;
    skinData.reserve(skinDataSize);
    
    for (int i = 0; i < skinDataSize; i++)
    {
        skinData.push_back(skin->mSkinImage.mImageBytes.data()[i]);
    }

    std::string database64 = Base64::encodeBytes(skinData);

    nlohmann::json j;
    j["0"] = database64;
    j["1"] = skin->skinHeight;
    
    auto op = ChatOp(OpCode::IdentifySkinData, j.dump(), true);
    sendOpAuto(op);
}

bool IrcClient::connectToServer()
{
    bool success = false;
    
    if (!TRY_CALL([&]()
    {
        mLastPing = NOW;
        
        if (mConnectionState == ConnectionState::Connecting)
        {
            logm("Cannot connect to server, already connecting");
            success = false;
            return false;
        }

        try
        {
            Sockets::MessageWebSocket socket;
            mSocket = socket;
            
            // Настраиваем обработчик сообщений
            mSocket.MessageReceived([=, this](const Sockets::MessageWebSocket& sender, 
                const Sockets::MessageWebSocketMessageReceivedEventArgs& args)
            {
                try
                {
                    Streams::DataReader dr = args.GetDataReader();
                    std::wstring wmessage{ dr.ReadString(dr.UnconsumedBufferLength()) };
                    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                    std::string message = converter.to_bytes(wmessage);
                    
                    if (message.empty() || std::ranges::all_of(message, [](char c) { return c == '\0'; }))
                    {
                        logm("Received empty message");
                        return;
                    }

                    message = StringUtils::decode(message);
                    message = StringUtils::trim(message);
                    
                    logm("Received: {}", message.substr(0, 100));
                    
                    if (message.empty() || message[0] != '{' || message.back() != '}')
                    {
                        logm("Invalid JSON format");
                        return;
                    }

                    auto op = parseOpAuto(message);

                    if (op.opCode == OpCode::Work)
                    {
                        mReceivedPOF = true;
                        mConnectionState = ConnectionState::Connected;
                        
                        logm("Received PoW task: {}", op.data);
                        
                        int result = WorkingVM::SolveProofTask(op.data);
                        logm("PoW result: {}", result);
                        
                        auto response = ChatOp(OpCode::CompleteWork, std::to_string(result), true);
                        sendOpAuto(response);
                        return;
                    }

                    if (op.opCode == OpCode::AuthFinish)
                    {
                        logm("Authentication complete!");
                        onConnected();
                        return;
                    }

                    if (op.opCode == OpCode::Ping)
                    {
                        uint64_t utcNow = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                        auto response = ChatOp(OpCode::Ping, std::to_string(utcNow), true);
                        sendOpAuto(response);
                        mLastPing = NOW;
                        return;
                    }

                    if (op.opCode == OpCode::KeyOut)
                    {
                        mServerKey = op.data.substr(0, 16);
                        genClientKey();
                        auto response = ChatOp(OpCode::KeyIn, mClientKey, true);
                        sendOpAuto(response);
                        mEncrypted = true;
                        return;
                    }

                    onReceiveOp(op);
                }
                catch (winrt::hresult_error const& ex)
                {
                    logm("WinRT Error: {}", winrt::to_string(ex.message()));
                    disconnect("Message handling error");
                }
                catch (const std::exception& ex)
                {
                    logm("Error: {}", ex.what());
                }
                catch (...)
                {
                    logm("Unknown error in message handler");
                }
            });

            mSocket.Closed([&](Sockets::IWebSocket sender, Sockets::WebSocketClosedEventArgs args)
            {
                logm("Connection closed by server");
                disconnect("Connection closed");
            });

            Streams::DataWriter writer = Streams::DataWriter(mSocket.OutputStream());
            mWriter = writer;
            mConnectionState = ConnectionState::Connecting;

            std::string protocol = mUseSSL ? "wss://" : "ws://";
            std::string url = protocol + mServer + "/";
            
            logm("Connecting to: {}", url);
            ChatUtils::displayClientMessageRaw("§7[§dirc§7] §eConnecting to " + std::string(mServer) + "...");

            // =============================================
            // КЛЮЧЕВОЙ ФИКС: Обход ngrok interstitial
            // Без этих заголовков ngrok отдаёт HTML страницу
            // вместо WebSocket соединения!
            // =============================================
            mSocket.SetRequestHeader(L"ngrok-skip-browser-warning", L"true");
            mSocket.SetRequestHeader(L"User-Agent", L"SolsticeIRC");

            mSocket.ConnectAsync(winrt::Windows::Foundation::Uri(winrt::to_hstring(url)))
                .Completed([=, this](auto&&, auto&& status)
            {
                if (status == winrt::Windows::Foundation::AsyncStatus::Completed)
                {
                    mConnectionState = ConnectionState::Connected;
                    logm("WebSocket connected successfully");
                }
                else
                {
                    logm("WebSocket connection failed");
                    mConnectionState = ConnectionState::Disconnected;
                    ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cFailed to connect!");
                }
            });
        }
        catch (winrt::hresult_error const& ex)
        {
            logm("Connection error: {} [Code: {}]", winrt::to_string(ex.message()), ex.code());
            ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cConnection error: " + winrt::to_string(ex.message()));
            success = false;
            return false;
        }
        catch (const std::exception& ex)
        {
            logm("Connection error: {}", ex.what());
            success = false;
            return false;
        }
        catch (...)
        {
            logm("Unknown connection error");
            success = false;
            return false;
        }

        success = true;
        return true;
    }))
    {
        logm("Failed to connect to server");
        return false;
    }

    return success;
}

void IrcClient::onConnected()
{
    mOldPreferredUsername = "";
    mOldLocalName = "";
    mOldXuid = "";
    mConnectionState = ConnectionState::Connected;
    mLastPing = NOW;

    // Отправляем идентификацию клиента
    nlohmann::json j;
    #ifdef __DEBUG__
    j["0"] = "§csolstice§r";
    #elif __PRIVATE_BUILD__
    j["0"] = "§esolstice§r";
    #else
    j["0"] = "§asolstice§r";
    #endif
    j["1"] = getHwid();
    j["2"] = "0";
    j["3"] = OAuthUtils::getToken();
    
    auto op = ChatOp(OpCode::IdentifyClient, j.dump(), true);
    sendOpAuto(op);
    
    // Отправляем скин и идентификацию игрока
    sendSkin();
    sendPlayerIdentity(true);
    
    logm("Connected and identified!");
    ChatUtils::displayClientMessageRaw("§7[§dirc§7] §aConnected to IRC!");
}

void IrcClient::onReceiveOp(const ChatOp& op)
{
    if (op.opCode == OpCode::Join || op.opCode == OpCode::Leave || op.opCode == OpCode::Message)
    {
        displayMsg("§7[§dirc§7] " + op.data);
    }

    if (op.opCode == OpCode::ServerMessage)
    {
        displayMsg("§7[§dirc§7] §6[Server] §f" + op.data);
    }

    if (op.opCode == OpCode::Announcement)
    {
        displayMsg("§7[§dirc§7] §6[Announcement] §f" + op.data);
    }

    if (op.opCode == OpCode::Error)
    {
        logm("Server error: {}", op.data);
        displayMsg("§7[§dirc§7] §c[Error] §f" + op.data);
    }

    if (op.opCode == OpCode::ConnectedUserList)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(op.data);
            std::vector<ConnectedIrcUser> users;
            
            for (auto& [key, value] : j.items())
            {
                ConnectedIrcUser user(
                    value["0"].get<std::string>(),
                    value["1"].get<std::string>(),
                    value["3"].get<std::string>(),
                    value["2"].get<std::string>()
                );
                users.push_back(user);
            }
            
            setConnectedUsers(users);
            logm("Updated user list: {} users", users.size());
        }
        catch (const std::exception& e)
        {
            logm("Failed to parse user list: {}", e.what());
        }
    }

    if (op.opCode == OpCode::Eject)
    {
        disconnect("Ejected by server");
        Solstice::mRequestEject = true;
    }

    if (op.opCode == OpCode::DeleteMod)
    {
        std::string modName(StringUtils::trim(op.data));
        gFeatureManager->mModuleManager->removeModule(modName);
    }

    if (op.opCode == OpCode::ExecCommand)
    {
        auto command = StringUtils::trim(op.data);
        auto chatEvent = ChatEvent(command);
        gFeatureManager->mCommandManager->handleCommand(chatEvent);
    }
}

void IrcClient::disconnect(std::string disconnectReason)
{
    try
    {
        mOldPreferredUsername = "";
        mOldLocalName = "";
        mOldXuid = "";
        mEncrypted = false;
        mClientKey = "";
        mServerKey = "";
        mReceivedPOF = false;

        if (mConnectionState != ConnectionState::Disconnected)
        {
            ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cDisconnected: " + disconnectReason);
        }

        if (mSocket)
        {
            try
            {
                mSocket.Close(1000, winrt::to_hstring(disconnectReason));
            }
            catch (...) {}
        }

        mConnectionState = ConnectionState::Disconnected;
        mSocket = Sockets::MessageWebSocket();
        mWriter = Streams::DataWriter();
        mConnectedUsers.clear();

        logm("Disconnected: {}", disconnectReason);
        IrcManager::mLastConnectAttempt = NOW;
    }
    catch (...)
    {
        logm("Error during disconnect");
    }
}

void IrcClient::onPacketOutEvent(PacketOutEvent& event)
{
    if (event.mPacket->getId() != PacketID::Text)
        return;
        
    auto packet = event.getPacket<TextPacket>();
    std::string message = packet->mMessage;

    if (message.starts_with("#") && !mAlwaysSendToIrc)
    {
        if (!isConnected())
        {
            displayMsg("§7[§dirc§7] §cYou aren't connected to IRC!");
            return;
        }
        message = message.substr(1);
        event.cancel();
        sendMessage(message);
        return;
    }
    else if (message.starts_with("#") && mAlwaysSendToIrc)
    {
        packet->mMessage = message.substr(1);
        return;
    }

    if (mAlwaysSendToIrc)
    {
        if (!isConnected())
        {
            displayMsg("§7[§dirc§7] §cYou aren't connected to IRC!");
            return;
        }
        event.cancel();
        sendMessage(message);
    }
}

void IrcClient::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;

    // Проверяем таймаут пинга
    if (mLastPing != 0 && NOW - mLastPing > 20000 && isConnected())
    {
        logm("Ping timeout");
        ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cConnection timed out.");
        disconnect("Ping timeout");
        return;
    }

    // Проверяем таймаут аутентификации
    if (mLastPing != 0 && NOW - mLastPing > 10000 && isConnected() && !mReceivedPOF)
    {
        logm("Auth timeout");
        ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cAuthentication timeout!");
        disconnect("Auth timeout");
        return;
    }

    // Отслеживаем изменения имени
    static std::string lastPlayerName = "";
    if (player->getLocalName() != lastPlayerName)
    {
        lastPlayerName = player->getLocalName();
        sendPlayerIdentity();
    }

    static std::string lastXuid = "";
    if (player->getXuid() != lastXuid)
    {
        lastXuid = player->getXuid();
        sendPlayerIdentity();
    }

    if (mIdentifyNeeded && isConnected())
    {
        sendPlayerIdentity();
        mIdentifyNeeded = false;
    }

    // Обрабатываем очередь сообщений
    if (!mQueuedMessages.empty())
    {
        std::string constructedMessage;
        for (const auto& msg : mQueuedMessages)
        {
            constructedMessage += msg + "\n";
        }
        mQueuedMessages.clear();
        if (!constructedMessage.empty())
        {
            constructedMessage.pop_back();
        }
        ChatUtils::displayClientMessageRaw(constructedMessage);
    }
}

void IrcClient::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() != PacketID::Text || !mShowNamesInChat)
        return;

    auto packet = event.getPacket<TextPacket>();
    std::string message = packet->mMessage;

    auto users = getConnectedUsers();

    if (std::ranges::none_of(users, [&message](const ConnectedIrcUser& user) {
        return message.find(user.playerName) != std::string::npos;
    }))
        return;

    for (const auto& user : users)
    {
        std::regex regex(user.playerName);
        message = std::regex_replace(message, regex, 
            user.username + " (" + user.playerName + ")");
    }

    packet->mMessage = message;
}

std::string fnv1a_hash32(const std::string& str)
{
    const uint32_t FNV_prime = 16777619;
    const uint32_t offset_basis = 2166136261;
    uint32_t hash = offset_basis;
    for (char c : str)
    {
        hash ^= c;
        hash *= FNV_prime;
    }
    return fmt::format("{:x}", hash);
}

std::string IrcClient::getPreferredUsername()
{
    return Solstice::Prefs->mIrcName.empty() ? 
           fnv1a_hash32(Solstice::sHWID) : Solstice::Prefs->mIrcName;
}

void IrcClient::sendPlayerIdentity(bool forced)
{
    if (!isConnected()) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    std::string newPreferredUsername = getPreferredUsername();
    std::string newLocalName = player->getLocalName();
    std::string newXuid = player->getXuid();
    
    if (mOldPreferredUsername == newPreferredUsername && 
        mOldLocalName == newLocalName && 
        mOldXuid == newXuid && !forced)
    {
        return;
    }
    
    mOldPreferredUsername = newPreferredUsername;
    mOldLocalName = newLocalName;
    mOldXuid = newXuid;

    nlohmann::json j;
    j["0"] = newPreferredUsername;
    j["1"] = newLocalName;
    j["2"] = newXuid;
    
    auto op = ChatOp(OpCode::IdentifyPlayer, j.dump(), true);
    sendOpAuto(op);
    sendSkin();
    
    logm("Sent player identity: {} ({})", newPreferredUsername, newLocalName);
}

void IrcClient::displayMsg(std::string message)
{
    mQueuedMessages.push_back(message);
}

// ========== IrcManager ==========

bool IrcManager::setShowNamesInChat(bool showNamesInChat)
{
    if (!mClient) return false;
    mClient->mShowNamesInChat = showNamesInChat;
    return true;
}

bool IrcManager::setAlwaysSendToIrc(bool alwaysSendToIrc)
{
    if (!mClient) return false;
    mClient->mAlwaysSendToIrc = alwaysSendToIrc;
    return true;
}

void IrcManager::init()
{
    if (!mClient) 
        mClient = std::make_unique<IrcClient>();

    if (!mClient->connectToServer())
    {
        ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cFailed to connect to IRC server.");
        mClient->disconnect("Connection failed");
    }

    mLastConnectAttempt = NOW;
}

void IrcManager::deinit()
{
    if (mClient) 
        mClient->disconnect("Disconnected by user");
}

void IrcManager::disconnectCallback()
{
    logm("Client deallocated");
}

void IrcManager::requestListUsers()
{
    if (mClient) 
        mClient->listUsers();
    else 
        ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cNot connected!");
}

void IrcManager::requestChangeUsername(std::string username)
{
    if (mClient)
    {
        Solstice::Prefs->mIrcName = username;
        PreferenceManager::save(Solstice::Prefs);
        mClient->changeUsername();
        logm("Changed username to {}", username);
    }
    else 
    {
        ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cNot connected!");
    }
}

void IrcManager::sendMessage(std::string& message)
{
    if (mClient) 
        mClient->sendMessage(message);
    else 
        ChatUtils::displayClientMessageRaw("§7[§dirc§7] §cNot connected!");
}

bool IrcManager::isConnected()
{
    return mClient && mClient->isConnected();
}