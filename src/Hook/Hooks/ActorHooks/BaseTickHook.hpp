#pragma once
//
// Created by vastrakai on 6/25/2024.
//

#include <Hook/Hook.hpp>
#include <SDK/Minecraft/Network/Packets/Packet.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>

#include <mutex>
#include <vector>

class QueuedPacket
{
public:
    uint64_t mTime;
    std::shared_ptr<Packet> mPacket;
    bool mBypssHook = true;

    QueuedPacket(std::shared_ptr<Packet> packet, bool bypassHook = true) : mPacket(packet), mBypssHook(bypassHook) {
        mTime = NOW;
    }
};

class BaseTickHook : public Hook {
public:
    BaseTickHook() : Hook() {
        mName = "Actor::baseTick";
    }

    /// <summary>
    /// General purpose packet queue.
    /// Can be used for various purposes, such as delaying packets to match rotation.
    /// </summary>
    static inline std::vector<QueuedPacket> mQueuedPackets;
    // mQueuedPackets пополняют из разных мест (и потоков), а отправляет их
    // onBaseTick — только под этим мьютексом (см. queuePacket).
    static inline std::mutex mPacketQueueMutex;
    static std::unique_ptr<Detour> mDetour;

    static void queuePacket(std::shared_ptr<Packet> packet, bool bypassHook)
    {
        std::lock_guard<std::mutex> lock(mPacketQueueMutex);
        mQueuedPackets.emplace_back(std::move(packet), bypassHook);
    }

    static void onBaseTick(class Actor* actor);
    void init() override;

    static inline std::vector<std::string> mQueuedMessages;
    static inline std::mutex mQueueMutex;

    static void queueMsg(const std::string& msg)
    {
        mQueueMutex.lock();
        mQueuedMessages.push_back(msg);
        mQueueMutex.unlock();
    }

    // should only be used by this hook
private:
    static void setMessageQueue(std::vector<std::string> messages)
    {
        mQueuedMessages = messages;
    }

    static std::vector<std::string> getMessageQueue()
    {
        auto messages = mQueuedMessages;
        return messages;
    }
};

