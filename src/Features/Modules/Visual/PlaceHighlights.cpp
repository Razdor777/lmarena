#include "PlaceHighlights.hpp"

#include <Features/FeatureManager.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>

void PlaceHighlights::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &PlaceHighlights::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &PlaceHighlights::onPacketOutEvent>(this);
}

void PlaceHighlights::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &PlaceHighlights::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &PlaceHighlights::onPacketOutEvent>(this);
}

void PlaceHighlights::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || ClientInstance::get()->getMouseGrabbed() || mPlaceMap.empty()) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    ImColor themedColor = ColorUtils::getThemedColor(0);

    const uint64_t now = NOW;
    const float duration = mDuration.mValue;
    const float maxOpacity = mMaxOpacity.mValue;

    // Прямой итератор: раньше позиции копировались во временный вектор ВРЕМЕНЕМ
    // и `mPlaceMap.erase(it)` стирал несуществующие ключи — записи не удалялись
    // никогда, и карта росла бесконечно.
    for (auto it = mPlaceMap.begin(); it != mPlaceMap.end();)
    {
        const float age = static_cast<float>(now - it->second);
        if (age >= duration)
        {
            it = mPlaceMap.erase(it);
            continue;
        }

        const float t = age / duration;            // 0..1
        const float fade = (1.f - t) * (1.f - t);  // мягкий хвост, а не «выключение»
        // всплеск появления: первые 140 мс бокс чуть крупнее и ярче
        const float pop = age < 140.f ? 1.f - age / 140.f : 0.f;

        const glm::vec3 center((float)it->first.x + 0.5f,
                               (float)it->first.y + 0.5f,
                               (float)it->first.z + 0.5f);
        const float halfSize = 0.5f * (1.f + 0.08f * pop);

        AABB boxAABB;
        boxAABB.mMin = center + glm::vec3(halfSize);
        boxAABB.mMax = center - glm::vec3(halfSize);

        std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(boxAABB);
        if (imPoints.size() < 3) { ++it; continue; }

        ImColor cColor = themedColor;
        if (mColorMode.mValue == ColorMode::Custom) cColor = mBoxColor.getAsImColor();

        // вспышка удара при установке — подмешиваем белый
        if (pop > 0.f)
        {
            cColor.Value.x += (1.f - cColor.Value.x) * pop * 0.7f;
            cColor.Value.y += (1.f - cColor.Value.y) * pop * 0.7f;
            cColor.Value.z += (1.f - cColor.Value.z) * pop * 0.7f;
        }

        if (mFilled.mValue)
        {
            ImColor fill = cColor;
            fill.Value.w = maxOpacity * fade;
            drawList->AddConvexPolyFilled(imPoints.data(), (int)imPoints.size(), fill);
        }

        // свечение: широкая мягкая обводка ПОД контуром
        ImColor glow = cColor;
        glow.Value.w = 0.35f * fade;
        drawList->AddPolyline(imPoints.data(), (int)imPoints.size(), glow, true, 5.0f);

        // контур ярче заливки — бокс не «мылится» при затухании
        ImColor line = cColor;
        const float lineAlpha = maxOpacity * fade * 1.6f;
        line.Value.w = (lineAlpha + pop * 0.3f > 1.f) ? 1.f : lineAlpha + pop * 0.3f;
        drawList->AddPolyline(imPoints.data(), (int)imPoints.size(), line, true, 2.0f);

        ++it;
    }
}

void PlaceHighlights::onPacketOutEvent(PacketOutEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    if (event.mPacket->getId() == PacketID::InventoryTransaction)
    {
        if (const auto it = event.getPacket<InventoryTransactionPacket>(); it->mTransaction->type ==
            ComplexInventoryTransaction::Type::ItemUseTransaction)
        {
            const auto transac = reinterpret_cast<ItemUseInventoryTransaction*>(it->mTransaction.get());
            if (transac->mActionType == ItemUseInventoryTransaction::ActionType::Place)
            {
                ItemStack* holdingItem = player->getSupplies()->getContainer()->getItem(transac->mSlot);
                if (holdingItem->mItem && holdingItem->mBlock)
                {
                    int face = transac->mFace;
                    if (face < 0 || 5 < face) {
                        return;
                    }
                    mPlaceMap[transac->mBlockPos + mOffsetList[transac->mFace]] = NOW;
                }
            }
        }
    }
}
