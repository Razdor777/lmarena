#include "ESP.hpp"

#include <Features/Modules/Player/Teams.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>

void ESP::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &ESP::onRenderEvent>(this);
}

void ESP::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ESP::onRenderEvent>(this);
}

ImColor ESP::getEntityColor(Actor* actor, float alpha)
{
    if (actor->isPlayer() && gFriendManager && gFriendManager->isFriend(actor))
    {
        ImColor col = mTeamColor.getAsImColor();
        col.Value.w = alpha;
        
        if (mShimmer.mValue)
        {
            float pulse = (sinf(ImGui::GetTime() * 2.0f) + 1.0f) * 0.5f;
            col.Value.x = std::min(1.0f, col.Value.x * (0.8f + 0.2f * pulse));
            col.Value.y = std::min(1.0f, col.Value.y * (0.8f + 0.2f * pulse));
            col.Value.z = std::min(1.0f, col.Value.z * (0.8f + 0.2f * pulse));
        }
        return col;
    }

    ImColor col;

    if (mSyncInterface.mValue)
    {
        auto interfaceMod = gFeatureManager->mModuleManager->getModule<Interface>();
        if (interfaceMod)
        {
            if (interfaceMod->mMode.mValue == Interface::Rainbow)
            {
                float speed = interfaceMod->mColorSpeed.mValue;
                float sat = interfaceMod->mSaturation.mValue;
                float hue = fmodf(ImGui::GetTime() * speed * 0.1f + ((uintptr_t)actor % 100) * 0.02f, 1.0f);
                col = ImColor::HSV(hue, sat, 1.0f, alpha);
            }
            else
            {
                auto themeColors = Interface::ColorThemes.find(interfaceMod->mMode.mValue);
                if (themeColors != Interface::ColorThemes.end() && !themeColors->second.empty())
                {
                    col = themeColors->second[1];
                    col.Value.w = alpha;
                }
                else
                {
                    col = mColor.getAsImColor();
                    col.Value.w = alpha;
                }
            }
        }
        else
        {
            col = mColor.getAsImColor();
            col.Value.w = alpha;
        }
    }
    else
    {
        col = mColor.getAsImColor();
        col.Value.w = alpha;
    }

    if (mShimmer.mValue)
    {
        float pulse = (sinf(ImGui::GetTime() * 2.0f) + 1.0f) * 0.5f;
        col.Value.x = std::min(1.0f, col.Value.x * (0.8f + 0.2f * pulse));
        col.Value.y = std::min(1.0f, col.Value.y * (0.8f + 0.2f * pulse));
        col.Value.z = std::min(1.0f, col.Value.z * (0.8f + 0.2f * pulse));
    }

    return col;
}

void ESP::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // === Фаза 1: безопасно собираем RuntimeID из списка акторов ===
    auto actorList = ActorUtils::getActorList(true, true);
    std::vector<int64_t> runtimeIds;
    runtimeIds.reserve(actorList.size());

    for (auto* a : actorList)
    {
        if (!a || a == player) continue;
        try {
            runtimeIds.push_back(a->getRuntimeID());
        } catch (...) {}
    }

    auto drawList = ImGui::GetBackgroundDrawList();

    // === Фаза 2: для каждого RuntimeID заново ищем актора в registry ===
    // Это гарантирует что мы работаем только с живыми акторами
    for (auto rid : runtimeIds)
    {
        auto* actor = ActorUtils::getActorFromRuntimeID(rid);
        if (!actor) continue;
        if (actor == player) continue;

        // Проверяем что у актора есть нужные компоненты
        if (!actor->getAABBShapeComponent()) continue;
        if (!actor->getRenderPositionComponent()) continue;

        if (!mInvisible.mValue && actor->getStatusFlag(ActorFlags::Invisible)) continue;

        AABB aabb = actor->getAABB();

        if (!std::isfinite(aabb.mMin.x) || !std::isfinite(aabb.mMin.y) || !std::isfinite(aabb.mMin.z)) continue;
        if (!std::isfinite(aabb.mMax.x) || !std::isfinite(aabb.mMax.y) || !std::isfinite(aabb.mMax.z)) continue;

        glm::vec3 corners[8] = {
            {aabb.mMin.x, aabb.mMin.y, aabb.mMin.z},
            {aabb.mMin.x, aabb.mMin.y, aabb.mMax.z},
            {aabb.mMax.x, aabb.mMin.y, aabb.mMin.z},
            {aabb.mMax.x, aabb.mMin.y, aabb.mMax.z},
            {aabb.mMin.x, aabb.mMax.y, aabb.mMin.z},
            {aabb.mMin.x, aabb.mMax.y, aabb.mMax.z},
            {aabb.mMax.x, aabb.mMax.y, aabb.mMin.z},
            {aabb.mMax.x, aabb.mMax.y, aabb.mMax.z}
        };

        ImVec2 screenCorners[8];
        bool valid = true;
        for (int i = 0; i < 8; i++)
        {
            if (!RenderUtils::worldToScreen(corners[i], screenCorners[i]))
            {
                valid = false;
                break;
            }
        }

        if (!valid) continue;

        float minX = FLT_MAX, maxX = -FLT_MAX, minY = FLT_MAX, maxY = -FLT_MAX;
        for (int i = 0; i < 8; i++)
        {
            minX = std::min(minX, screenCorners[i].x);
            maxX = std::max(maxX, screenCorners[i].x);
            minY = std::min(minY, screenCorners[i].y);
            maxY = std::max(maxY, screenCorners[i].y);
        }

        if (maxX - minX < 1.f || maxY - minY < 1.f) continue;

        float alpha = 1.0f;
        ImColor color = getEntityColor(actor, alpha);
        ImColor shadowColor = ImColor(0.0f, 0.0f, 0.0f, 0.5f);
        float width = mLineWidth.mValue;

        if (mBoxMode.mValue == BoxMode::Full2D)
        {
            if (mRenderFilled.mValue)
            {
                ImColor fillColor = ImColor(color.Value.x, color.Value.y, color.Value.z, 0.15f);
                drawList->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX, maxY), fillColor, 0.0f, 0);
            }

            if (mShadow.mValue)
            {
                drawList->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), shadowColor, 0.0f, 0, width + 2.0f);
            }

            drawList->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), color, 0.0f, 0, width);
        }
        else if (mBoxMode.mValue == BoxMode::Corners2D)
        {
            float boxW = maxX - minX;
            float boxH = maxY - minY;
            float cornerX = boxW * mCornerSize.mValue;
            float cornerY = boxH * mCornerSize.mValue;

            if (mRenderFilled.mValue)
            {
                ImColor fillColor = ImColor(color.Value.x, color.Value.y, color.Value.z, 0.15f);
                drawList->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX, maxY), fillColor, 0.0f, 0);
            }

            if (mShadow.mValue)
            {
                float sw = width + 2.0f;
                drawList->AddLine(ImVec2(minX, minY), ImVec2(minX + cornerX, minY), shadowColor, sw);
                drawList->AddLine(ImVec2(minX, minY), ImVec2(minX, minY + cornerY), shadowColor, sw);
                drawList->AddLine(ImVec2(maxX, minY), ImVec2(maxX - cornerX, minY), shadowColor, sw);
                drawList->AddLine(ImVec2(maxX, minY), ImVec2(maxX, minY + cornerY), shadowColor, sw);
                drawList->AddLine(ImVec2(minX, maxY), ImVec2(minX + cornerX, maxY), shadowColor, sw);
                drawList->AddLine(ImVec2(minX, maxY), ImVec2(minX, maxY - cornerY), shadowColor, sw);
                drawList->AddLine(ImVec2(maxX, maxY), ImVec2(maxX - cornerX, maxY), shadowColor, sw);
                drawList->AddLine(ImVec2(maxX, maxY), ImVec2(maxX, maxY - cornerY), shadowColor, sw);
            }

            drawList->AddLine(ImVec2(minX, minY), ImVec2(minX + cornerX, minY), color, width);
            drawList->AddLine(ImVec2(minX, minY), ImVec2(minX, minY + cornerY), color, width);
            drawList->AddLine(ImVec2(maxX, minY), ImVec2(maxX - cornerX, minY), color, width);
            drawList->AddLine(ImVec2(maxX, minY), ImVec2(maxX, minY + cornerY), color, width);
            drawList->AddLine(ImVec2(minX, maxY), ImVec2(minX + cornerX, maxY), color, width);
            drawList->AddLine(ImVec2(minX, maxY), ImVec2(minX, maxY - cornerY), color, width);
            drawList->AddLine(ImVec2(maxX, maxY), ImVec2(maxX - cornerX, maxY), color, width);
            drawList->AddLine(ImVec2(maxX, maxY), ImVec2(maxX, maxY - cornerY), color, width);
        }
        else if (mBoxMode.mValue == BoxMode::Box3D)
        {
            if (mRenderFilled.mValue)
            {
                auto imPoints = MathUtils::getImBoxPoints(aabb);
                if (imPoints.size() >= 3)
                {
                    ImColor fillColor = ImColor(color.Value.x, color.Value.y, color.Value.z, 0.15f);
                    drawList->AddConvexPolyFilled(imPoints.data(), (int)imPoints.size(), fillColor);
                }
            }

            if (mShadow.mValue)
            {
                float sw = width + 2.0f;
                drawList->AddLine(screenCorners[0], screenCorners[1], shadowColor, sw);
                drawList->AddLine(screenCorners[1], screenCorners[3], shadowColor, sw);
                drawList->AddLine(screenCorners[3], screenCorners[2], shadowColor, sw);
                drawList->AddLine(screenCorners[2], screenCorners[0], shadowColor, sw);
                drawList->AddLine(screenCorners[4], screenCorners[5], shadowColor, sw);
                drawList->AddLine(screenCorners[5], screenCorners[7], shadowColor, sw);
                drawList->AddLine(screenCorners[7], screenCorners[6], shadowColor, sw);
                drawList->AddLine(screenCorners[6], screenCorners[4], shadowColor, sw);
                drawList->AddLine(screenCorners[0], screenCorners[4], shadowColor, sw);
                drawList->AddLine(screenCorners[1], screenCorners[5], shadowColor, sw);
                drawList->AddLine(screenCorners[2], screenCorners[6], shadowColor, sw);
                drawList->AddLine(screenCorners[3], screenCorners[7], shadowColor, sw);
            }

            drawList->AddLine(screenCorners[0], screenCorners[1], color, width);
            drawList->AddLine(screenCorners[1], screenCorners[3], color, width);
            drawList->AddLine(screenCorners[3], screenCorners[2], color, width);
            drawList->AddLine(screenCorners[2], screenCorners[0], color, width);
            drawList->AddLine(screenCorners[4], screenCorners[5], color, width);
            drawList->AddLine(screenCorners[5], screenCorners[7], color, width);
            drawList->AddLine(screenCorners[7], screenCorners[6], color, width);
            drawList->AddLine(screenCorners[6], screenCorners[4], color, width);
            drawList->AddLine(screenCorners[0], screenCorners[4], color, width);
            drawList->AddLine(screenCorners[1], screenCorners[5], color, width);
            drawList->AddLine(screenCorners[2], screenCorners[6], color, width);
            drawList->AddLine(screenCorners[3], screenCorners[7], color, width);
        }
    }
}