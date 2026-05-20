//
// Created by vastrakai on 8/7/2024.
//

#include "ItemESP.hpp"

#include <Features/Events/BaseTickEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/ItemActor.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Features/Modules/Player/InvManager.hpp>

static std::vector<ItemInfo> gItems;
static std::mutex            gItemMutex;

void ItemESP::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &ItemESP::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &ItemESP::onBaseTickEvent>(this);
}

void ItemESP::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ItemESP::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &ItemESP::onBaseTickEvent>(this);
}

void ItemESP::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    std::vector<ItemInfo> newItems;

    auto actors = ActorUtils::getActorsTyped<ItemActor>(ActorType::ItemEntity);

    for (auto actor : actors)
    {
        if (!actor) continue;
        if (!actor->getStateVectorComponent()) continue;

        if (mDistanceLimited.mValue && player->distanceTo(actor) > mDistance.mValue)
            continue;

        auto renderPosComp = actor->getRenderPositionComponent();
        if (!renderPosComp) continue;

        auto shape = actor->getAABBShapeComponent();
        if (!shape) continue;

        // Защита от nullptr: проверяем И указатель, И объект
        if (!actor->mItem.mItem) continue;
        Item* itemPtr = actor->mItem.getItem();
        if (!itemPtr) continue;

        glm::vec3 pos        = renderPosComp->mPosition;
        float     aabbHeight = shape->mHeight;
        float     aabbWidth  = shape->mWidth;

        ItemStack* stack = &actor->mItem;
        bool        isUseful = !InvManager::isItemUseless(stack, -1);
        std::string name     = itemPtr->mName;

        if (name.empty()) continue;

        name += " x" + std::to_string(static_cast<int>(stack->mCount));

        std::vector<std::pair<std::string, int>> enchants;
        for (int i = 0; i <= static_cast<int>(Enchant::SWIFT_SNEAK); ++i)
        {
            int enchantValue = stack->getEnchantValue(i);
            if (enchantValue > 0)
            {
                enchants.emplace_back(
                    stack->getEnchantName(static_cast<Enchant>(i)),
                    enchantValue
                );
            }
        }

        newItems.push_back({ pos, { aabbWidth, aabbHeight }, name, isUseful, enchants });
    }

    std::lock_guard<std::mutex> lock(gItemMutex);
    gItems = std::move(newItems);
}

void ItemESP::onRenderEvent(RenderEvent& event)
{
    if (!ClientInstance::get()->getLevelRenderer()) return;

    auto ci     = ClientInstance::get();
    auto player = ci->getLocalPlayer();
    if (!player) return;

    std::vector<ItemInfo> localItems;
    {
        std::lock_guard<std::mutex> lock(gItemMutex);
        localItems = gItems;
    }

    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    for (const auto& item : localItems)
    {
        glm::vec3 ppos = item.pos;
        ppos -= glm::vec3(item.size.x / 2.f, 0.f, item.size.x / 2.f);

        AABB aabb;
        aabb.mMin = ppos;
        aabb.mMax = ppos + glm::vec3(item.size.x, item.size.y, item.size.x);

        std::vector<ImVec2> imPoints = MathUtils::getImBoxPoints(aabb);
        if (imPoints.empty()) continue;

        ImColor themeColor = mThemedColor.mValue
            ? ColorUtils::getThemedColor(0)
            : ImColor(1.0f, 1.0f, 1.0f);

        if (mRenderFilled.mValue)
            drawList->AddConvexPolyFilled(
                imPoints.data(),
                static_cast<int>(imPoints.size()),
                ImColor(themeColor.Value.x, themeColor.Value.y, themeColor.Value.z, 0.25f)
            );

        drawList->AddPolyline(
            imPoints.data(),
            static_cast<int>(imPoints.size()),
            themeColor,
            ImDrawFlags_Closed,
            2.0f
        );

        if (!mShowNames.mValue) continue;

        glm::vec3 pos    = item.pos;
        glm::vec3 origin = RenderUtils::transform.mOrigin;
        ImVec2 screen;

        if (!RenderUtils::worldToScreen(pos, screen))
            continue;

        if (std::isnan(screen.x) || std::isnan(screen.y)) continue;

        std::string name = item.name;

        if (mShowEnchant.mValue && !item.enchants.empty())
        {
            std::string enchantText;
            for (const auto& enchant : item.enchants)
            {
                if (!enchantText.empty()) enchantText += ", ";
                enchantText += "[" + enchant.first + "]";
            }
            name += " " + enchantText;
        }

        // === Расчёт размера шрифта (с защитой от NaN и взрыва) ===
        float fontSize = mFontSize.mValue;

        if (mDistanceScaledFont.mValue)
        {
            float distance = glm::distance(origin, pos) + 2.5f;

            if (std::isnan(distance) || distance < 0.001f)
                distance = 0.001f;

            fontSize = (1.0f / distance) * 100.0f * mScalingMultiplier.mValue;

            if (fontSize < 1.0f) fontSize = 1.0f;
            if (fontSize > mFontSize.mValue * 2.0f) fontSize = mFontSize.mValue * 2.0f;
        }

        // === Пушим шрифт ТОЛЬКО после всех проверок ===
        FontHelper::pushPrefFont(true, true);

        ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, name.c_str());
        ImVec2 textPos  = ImVec2(screen.x - textSize.x / 2.f, screen.y - textSize.y - 5.f);

        ImColor textCol = (item.isUseful && mHighlightUsefulItems.mValue)
            ? ImColor(0.0f, 1.0f, 0.0f)
            : ImColor(1.0f, 1.0f, 1.0f);

        ImRenderUtils::drawShadowText(drawList, name, textPos, textCol, fontSize, true);

        FontHelper::popPrefFont();
    }
}