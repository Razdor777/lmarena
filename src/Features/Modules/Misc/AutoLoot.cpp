#include "AutoLoot.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/MouseEvent.hpp>
#include <Features/Modules/Player/InvManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ItemActor.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

// =========================================================
// ENABLE / DISABLE
// =========================================================

void AutoLoot::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AutoLoot::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &AutoLoot::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<MouseEvent, &AutoLoot::onMouseEvent>(this);

    mState = LootState::WaitingFirstTP;
    mSavedPosition = glm::vec3(0.f);
    mPendingDisable = false;

    std::lock_guard<std::mutex> lock(mMutex);
    mPacketPositions.clear();
}

void AutoLoot::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AutoLoot::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &AutoLoot::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<MouseEvent, &AutoLoot::onMouseEvent>(this);

    mState = LootState::Returned;
    mPendingDisable = false;

    std::lock_guard<std::mutex> lock(mMutex);
    mPacketPositions.clear();
}

// =========================================================
// Create MovePlayerPacket
// =========================================================

std::shared_ptr<MovePlayerPacket> AutoLoot::createPacketForPos(Actor* player, glm::vec3 pos)
{
    auto packet = MinecraftPackets::createPacket<MovePlayerPacket>();
    packet->mPos = pos;
    packet->mPlayerID = player->getRuntimeID();

    auto rot = player->getActorRotationComponent();
    if (rot)
    {
        packet->mRot = {rot->mPitch, rot->mYaw};
        packet->mYHeadRot = rot->mYaw;
    }

    packet->mResetPosition = PositionMode::Teleport;
    packet->mOnGround = true;
    packet->mRidingID = -1;
    packet->mCause = TeleportationCause::Unknown;
    packet->mSourceEntityType = ActorType::Player;
    packet->mTick = 0;
    return packet;
}

// =========================================================
// Straight line TP
// =========================================================

void AutoLoot::straightLineTP(Actor* player, glm::vec3 from, glm::vec3 to, bool saveForRender)
{
    auto sender = ClientInstance::get()->getPacketSender();
    if (!sender) return;

    float stepSize = mStepDistance.mValue;
    glm::vec3 diff = to - from;
    float totalDist = glm::length(diff);

    if (totalDist < 0.01f)
    {
        sender->sendToServer(createPacketForPos(player, to).get());
        return;
    }

    glm::vec3 direction = glm::normalize(diff);
    glm::vec3 currentPos = from;
    std::vector<glm::vec3> positions;

    while (glm::distance(currentPos, to) > stepSize)
    {
        currentPos += direction * stepSize;
        positions.push_back(currentPos);
        sender->sendToServer(createPacketForPos(player, currentPos).get());
    }

    positions.push_back(to);
    sender->sendToServer(createPacketForPos(player, to).get());

    if (saveForRender)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPacketPositions.clear();
        for (auto& p : positions)
            mPacketPositions.push_back(p);
        mLastPathTime = NOW;
    }
}

// =========================================================
// Find nearest item and TP to it
// Returns true if found and teleported
// =========================================================

bool AutoLoot::findAndTeleportToNearest(Actor* player, glm::vec3 fromPos)
{
    if (!player) return false;

    auto itemActors = ActorUtils::getActorsTyped<ItemActor>(ActorType::ItemEntity);

    struct ItemTarget {
        glm::vec3 pos;
        float distance;
    };

    std::vector<ItemTarget> targets;

    for (auto item : itemActors)
    {
        if (!item) continue;
        if (!item->mItem.mItem) continue;
        if (!item->getStateVectorComponent()) continue;

        glm::vec3 itemPos = item->getStateVectorComponent()->mPos;
        float dist = glm::distance(fromPos, itemPos);

        // Skip items that are very close (will be picked up naturally)
        if (dist < 3.f) continue;

        // Range check from original saved position
        float distFromHome = glm::distance(mSavedPosition, itemPos);
        if (!mInfiniteRange.mValue && distFromHome > mRange.mValue) continue;

        if (mOnlyUseful.mValue)
        {
            if (InvManager::isItemUseless(&item->mItem, -1)) continue;
        }

        targets.push_back({itemPos, dist});
    }

    if (targets.empty()) return false;

    // Sort by distance from current position
    std::ranges::sort(targets, [](const ItemTarget& a, const ItemTarget& b) {
        return a.distance < b.distance;
    });

    auto& nearest = targets[0];

    // Safe Y: item is on ground level, packet pos = eye level
    // feet should be at item level → eye = item.y + PLAYER_HEIGHT
    glm::vec3 safePos = glm::vec3(
        nearest.pos.x,
        nearest.pos.y + PLAYER_HEIGHT,
        nearest.pos.z
    );

    // Teleport
    straightLineTP(player, fromPos, safePos, true);

    // Set position and clear velocity for smooth landing
    player->setPosition(safePos);

    auto stateVec = player->getStateVectorComponent();
    if (stateVec)
        stateVec->mVelocity = glm::vec3(0.f, 0.f, 0.f);

    return true;
}

// =========================================================
// Teleport back to saved position
// =========================================================

void AutoLoot::teleportBack(Actor* player)
{
    if (!player) return;

    glm::vec3 currentPos = *player->getPos();

    straightLineTP(player, currentPos, mSavedPosition, true);

    player->setPosition(mSavedPosition);

    auto stateVec = player->getStateVectorComponent();
    if (stateVec)
        stateVec->mVelocity = glm::vec3(0.f, 0.f, 0.f);

    mState = LootState::Returned;

    // Schedule disable on next tick (can't disable from event handler)
    mPendingDisable = true;
}

// =========================================================
// MOUSE EVENT
// =========================================================

void AutoLoot::onMouseEvent(MouseEvent& event)
{
    if (mState != LootState::AtItem) return;
    if (!mTPBack.mValue) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // LMB (1) pressed (1) → teleport back + disable module
    if (event.mActionButtonId == 1 && event.mButtonData == 1)
    {
        teleportBack(player);
        event.mCancelled = true;
        return;
    }

    // RMB (2) pressed (1) → teleport to next item
    if (event.mActionButtonId == 2 && event.mButtonData == 1)
    {
        glm::vec3 currentPos = *player->getPos();

        if (findAndTeleportToNearest(player, currentPos))
        {
            // Found next item, stay AtItem
            mState = LootState::AtItem;
        }
        // If no items found, stay where we are

        event.mCancelled = true;
        return;
    }
}

// =========================================================
// MAIN TICK
// =========================================================

void AutoLoot::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    // Handle pending disable (from teleportBack)
    if (mPendingDisable)
    {
        mPendingDisable = false;
        setEnabled(false);
        return;
    }

    // =========================================================
    // WaitingFirstTP — first tick, find nearest item and TP
    // =========================================================
    if (mState == LootState::WaitingFirstTP)
    {
        glm::vec3 playerPos = *player->getPos();
        mSavedPosition = playerPos;

        if (findAndTeleportToNearest(player, playerPos))
        {
            if (mTPBack.mValue)
            {
                // Teleported — now player can walk around freely
                // LMB = return home + disable
                // RMB = next item
                mState = LootState::AtItem;
            }
            else
            {
                // No TP back — just teleported, disable
                mState = LootState::Returned;
                mPendingDisable = true;
            }
        }
        // else: no items found, keep scanning each tick
        return;
    }

    // =========================================================
    // AtItem — player walks freely, no position lock
    // Just waiting for LMB/RMB input
    // =========================================================
    if (mState == LootState::AtItem)
    {
        // Nothing! Player moves freely.
        // Mouse event handler takes care of LMB/RMB
        return;
    }

    // =========================================================
    // Returned — nothing to do, module will disable itself
    // =========================================================
}

// =========================================================
// RENDER
// =========================================================

void AutoLoot::onRenderEvent(RenderEvent& event)
{
    if (!mDrawPath.mValue) return;

    auto ci = ClientInstance::get();
    auto player = ci->getLocalPlayer();
    if (!player) return;

    float alphaMultiplier = 1.0f;
    uint64_t fadeTime = 2000;
    uint64_t currentTime = NOW;

    std::lock_guard<std::mutex> lock(mMutex);

    if (mState == LootState::AtItem)
    {
        // Keep path visible while at item
        alphaMultiplier = 1.0f;
    }
    else
    {
        if (mLastPathTime + fadeTime < currentTime)
        {
            mPacketPositions.clear();
            return;
        }
        alphaMultiplier = 1.0f - static_cast<float>(currentTime - mLastPathTime) / static_cast<float>(fadeTime);
        alphaMultiplier = std::clamp(alphaMultiplier, 0.0f, 1.0f);
    }

    if (mPacketPositions.empty()) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    std::vector<ImVec2> points;

    // Start point = saved position (home)
    ImVec2 startScreen;
    if (RenderUtils::worldToScreen(mSavedPosition, startScreen))
        points.push_back(startScreen);

    for (auto& pos : mPacketPositions)
    {
        ImVec2 point;
        if (!RenderUtils::worldToScreen(pos, point)) continue;
        points.emplace_back(point);
    }

    if (points.size() < 2) return;

    // Lines
    for (size_t i = 0; i < points.size() - 1; i++)
    {
        ImColor color = ImColor(0.2f, 1.0f, 0.4f, alphaMultiplier);
        drawList->AddLine(points[i], points[i + 1], color, 2.0f);
    }

    // Dots
    if (!points.empty())
    {
        drawList->AddCircleFilled(points[0], 5.0f, ImColor(0.2f, 1.0f, 0.2f, alphaMultiplier));
        drawList->AddCircleFilled(points.back(), 5.0f, ImColor(1.0f, 1.0f, 0.2f, alphaMultiplier));
    }

    // HUD while at item
    if (mState == LootState::AtItem && mTPBack.mValue)
    {
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        float centerX = screenSize.x * 0.5f;
        float topY = screenSize.y * 0.30f;

        const char* line1 = "LMB - Return + Disable";
        const char* line2 = "RMB - Next Item";

        ImVec2 size1 = ImGui::CalcTextSize(line1);
        ImVec2 size2 = ImGui::CalcTextSize(line2);

        float boxW = std::max(size1.x, size2.x) + 20.f;
        float boxH = size1.y + size2.y + 15.f;
        ImVec2 boxMin = ImVec2(centerX - boxW * 0.5f, topY - 5.f);
        ImVec2 boxMax = ImVec2(centerX + boxW * 0.5f, topY + boxH);
        drawList->AddRectFilled(boxMin, boxMax, ImColor(0.0f, 0.0f, 0.0f, 0.6f * alphaMultiplier), 6.0f);

        drawList->AddText(
            ImVec2(centerX - size1.x * 0.5f, topY),
            ImColor(0.4f, 1.0f, 0.4f, alphaMultiplier), line1
        );
        drawList->AddText(
            ImVec2(centerX - size2.x * 0.5f, topY + size1.y + 5.f),
            ImColor(1.0f, 1.0f, 0.4f, alphaMultiplier), line2
        );
    }

    // Scanning text
    if (mState == LootState::WaitingFirstTP)
    {
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        float centerX = screenSize.x * 0.5f;
        float topY = screenSize.y * 0.35f;

        const char* text = "Scanning for items...";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        drawList->AddText(
            ImVec2(centerX - textSize.x * 0.5f, topY),
            ImColor(1.0f, 1.0f, 1.0f, 0.8f), text
        );
    }
}