//
// Created by vastrakai on 8/31/2024.
//

#include "SkinStealer.hpp"

#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerSkinPacket.hpp>
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <Utils/stb_image_write.h>
#include "stb_image.h"
#include <filesystem>
#include <chrono>

std::vector<uint8_t> SkinStealer::convToPng(const std::vector<uint8_t>& data, int width, int height)
{
    std::vector<uint8_t> pngData;

    // Callback function to store the PNG data in a vector
    auto writeToVector = [](void* context, void* data, int size) {
        std::vector<uint8_t>* pngData = static_cast<std::vector<uint8_t>*>(context);
        pngData->insert(pngData->end(), (uint8_t*)data, (uint8_t*)data + size);
    };

    // Convert the raw RGBA data to PNG format and store it in the vector
    if (stbi_write_png_to_func(writeToVector, &pngData, width, height, 4, data.data(), width * 4) == 0) {
        // Handle error: PNG conversion failed
        return {};
    }

    return pngData;
}

std::vector<unsigned char> converToRGBA(const std::vector<unsigned char>& data, int& width, int& height, int& depth)
{
    unsigned char* imgData = stbi_load_from_memory(data.data(), data.size(), &width, &height, &depth, 4);
    if (imgData == nullptr)
    {
        spdlog::error("Failed to load image from memory");
        return {};
    }

    std::vector<unsigned char> rgbaData(width * height * 4);
    for (int i = 0; i < width * height; i++)
    {
        rgbaData[i * 4] = imgData[i * 4];
        rgbaData[i * 4 + 1] = imgData[i * 4 + 1];
        rgbaData[i * 4 + 2] = imgData[i * 4 + 2];
        rgbaData[i * 4 + 3] = imgData[i * 4 + 3];
    }

    stbi_image_free(imgData);
    return rgbaData;
}

void SkinStealer::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &SkinStealer::onBaseTickEvent>(this);
}

void SkinStealer::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &SkinStealer::onBaseTickEvent>(this);
}

void SkinStealer::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    EntityId targetId = player->getLevel()->getHitResult()->mEntity.id;

    Actor* targeted = nullptr;
    for (auto actor : ActorUtils::getActorList())
    {
        if (actor->mContext.mEntityId == targetId)
        {
            targeted = actor;
            break;
        }
    }

    bool rightClick = ImGui::IsMouseDown(1);
    static bool lastRightClick = false;

    if (rightClick && !lastRightClick && targeted && targeted->isPlayer())
    {
        saveSkin(targeted);
    }

    lastRightClick = rightClick;
}

void SkinStealer::saveSkin(Actor* actor)
{
    auto skin = actor->getSkin();
    if (!skin) return;

    const uint8_t* skinData = skin->mSkinImage.mImageBytes.data();
    int width = skin->mSkinImage.mWidth;
    int height = skin->mSkinImage.mHeight;
    int bytes = width * height * 4;
    if (bytes <= 0 || width <= 0 || height <= 0) return;

    std::vector<uint8_t> pngData = convToPng(std::vector<uint8_t>(skinData, skinData + bytes), width, height);
    if (pngData.empty()) return;

    // --- Ensure Skins directory exists ---
    std::string skinsDir = FileUtils::getSolsticeDir() + "Skins\\";
    try {
        std::filesystem::create_directories(skinsDir);
    } catch (...) {
        ChatUtils::displayClientMessage("§cFailed to create Skins directory!");
        return;
    }

    // --- Generate timestamp for unique filename ---
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::tm tm_buf;
    localtime_s(&tm_buf, &time_t_now);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &tm_buf);
    std::string timestamp = std::string(timeBuf) + "_" + std::to_string(ms.count());

    std::string playerName = actor->getRawName();
    std::string skinPath = skinsDir + playerName + "_skin_" + timestamp + ".png";

    std::ofstream file(skinPath, std::ios::binary);
    if (!file.is_open()) {
        ChatUtils::displayClientMessage("§cFailed to save skin file!");
        return;
    }
    file.write((char*)pngData.data(), pngData.size());
    file.close();

    // --- Cape ---
    const uint8_t* capeData = skin->mCapeImage.mImageBytes.data();
    int capeWidth = skin->mCapeImage.mWidth;
    int capeHeight = skin->mCapeImage.mHeight;
    int capeBytes = capeWidth * capeHeight * 4;

    std::vector<uint8_t> capePngData;
    if (capeBytes > 0) {
        capePngData = convToPng(std::vector<uint8_t>(capeData, capeData + capeBytes), capeWidth, capeHeight);
        if (!capePngData.empty()) {
            std::string capePath = skinsDir + playerName + "_cape_" + timestamp + ".png";
            std::ofstream capeFile(capePath, std::ios::binary);
            if (capeFile.is_open()) {
                capeFile.write((char*)capePngData.data(), capePngData.size());
                capeFile.close();
            }
        }
    } else {
        ChatUtils::displayClientMessage("No cape found for " + playerName);
    }

    ChatUtils::displayClientMessage("§aSaved skin for §b" + playerName + "§a to Solstice\\Skins\\ (copied to clipboard)");
    ImGui::SetClipboardText(skinsDir.c_str());

    std::string resourcePatch = skin->mResourcePatch;
    if (mApplySkin) applySkin(actor, pngData, capePngData, capeWidth, capeHeight, resourcePatch);
}

void SkinStealer::applySkin(Actor* actor, const std::vector<uint8_t>& skinData, const std::vector<uint8_t>& capeData, int capeWidth, int capeHeight, const std::string& resourcePatch)
{
    try {
        auto player = ClientInstance::get()->getLocalPlayer();
        if (!player) {
            ChatUtils::displayClientMessage("§cFailed to apply skin: no local player.");
            return;
        }

        auto targetSkin = actor->getSkin();
        if (!targetSkin) {
            ChatUtils::displayClientMessage("§cFailed to apply skin: target has no skin.");
            return;
        }

        auto currentSkin = player->getSkin();
        if (!currentSkin) {
            ChatUtils::displayClientMessage("§cFailed to apply skin: couldn't get current skin.");
            return;
        }

        // Validate target skin image
        if (targetSkin->mSkinImage.mWidth <= 0 || targetSkin->mSkinImage.mHeight <= 0 ||
            targetSkin->mSkinImage.mImageBytes.size() == 0) {
            ChatUtils::displayClientMessage("§cFailed to apply skin: target skin image is invalid.");
            return;
        }

        // ── Build packet ────────────────────────────────────────────────
        auto skinPacket = MinecraftPackets::createPacket<PlayerSkinPacket>();
        if (!skinPacket) {
            ChatUtils::displayClientMessage("§cFailed to create skin packet.");
            return;
        }

        // UUID — find local player's UUID from player list
        mce::UUID localUUID = mce::UUID();
        auto level = player->getLevel();
        if (level) {
            auto playerList = level->getPlayerList();
            if (playerList) {
                std::string localName = player->getRawName();
                for (auto& [uuid, entry] : *playerList) {
                    if (entry.mName == localName) {
                        localUUID = uuid;
                        break;
                    }
                }
            }
        }
        skinPacket->mUUID = localUUID;

        // ── Clone the ENTIRE skin from target directly ──
        // This avoids all the PNG→RGBA→back conversion that was crashing.
        // We copy the raw skin data as-is from the target actor's skin.
        skinPacket->mSkin.mId               = currentSkin->mId;       // Keep OUR skin ID
        skinPacket->mSkin.mPlayFabId        = currentSkin->mPlayFabId;
        skinPacket->mSkin.mFullId           = currentSkin->mFullId;
        skinPacket->mSkin.mCapeId           = targetSkin->mCapeId;    // Use target's cape ID

        // Copy skin image DIRECTLY from target (raw RGBA, NOT PNG)
        skinPacket->mSkin.mSkinImage.mImageBytes = targetSkin->mSkinImage.mImageBytes;
        skinPacket->mSkin.mSkinImage.mWidth      = targetSkin->mSkinImage.mWidth;
        skinPacket->mSkin.mSkinImage.mHeight     = targetSkin->mSkinImage.mHeight;
        skinPacket->mSkin.mSkinImage.mDepth      = 4;

        // Copy cape image DIRECTLY from target
        if (targetSkin->mCapeImage.mWidth > 0 && targetSkin->mCapeImage.mHeight > 0 &&
            targetSkin->mCapeImage.mImageBytes.size() > 0) {
            skinPacket->mSkin.mCapeImage.mImageBytes = targetSkin->mCapeImage.mImageBytes;
            skinPacket->mSkin.mCapeImage.mWidth      = targetSkin->mCapeImage.mWidth;
            skinPacket->mSkin.mCapeImage.mHeight     = targetSkin->mCapeImage.mHeight;
            skinPacket->mSkin.mCapeImage.mDepth      = 4;
        }

        // Geometry — copy from target
        skinPacket->mSkin.mResourcePatch       = targetSkin->mResourcePatch;
        skinPacket->mSkin.mDefaultGeometryName = targetSkin->mDefaultGeometryName;
        skinPacket->mSkin.mArmSizeType         = targetSkin->mArmSizeType;

        // Flags
        skinPacket->mSkin.mIsTrustedSkin                 = TrustedSkinFlag::YES;
        skinPacket->mSkin.mIsPremium                     = false;
        skinPacket->mSkin.mIsPersona                     = false;
        skinPacket->mSkin.mIsPersonaCapeOnClassicSkin     = false;
        skinPacket->mSkin.mIsPrimaryUser                 = true;
        skinPacket->mSkin.mOverridesPlayerAppearance     = false;

        // Send to server
        auto sender = ClientInstance::get()->getPacketSender();
        if (!sender) {
            ChatUtils::displayClientMessage("§cFailed to apply skin: no packet sender.");
            return;
        }
        sender->sendToServer(skinPacket.get());
        ChatUtils::displayClientMessage("§aSuccessfully applied skin from §b" + actor->getRawName() + "§a.");

    } catch (const std::exception& e) {
        ChatUtils::displayClientMessage(std::string("§cSkin apply crash prevented: ") + e.what());
        spdlog::error("[SkinStealer] applySkin exception: {}", e.what());
    } catch (...) {
        ChatUtils::displayClientMessage("§cSkin apply crash prevented (unknown error).");
        spdlog::error("[SkinStealer] applySkin unknown exception");
    }
}