//
// AutoPatrol - Automatically patrol between pos1 and pos2
//

#include "AutoPatrol.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <Features/Modules/Movement/AutoPath.hpp>

// Инициализация статических переменных
glm::vec3 AutoPatrol::sPos1 = glm::vec3(0.0f);
glm::vec3 AutoPatrol::sPos2 = glm::vec3(0.0f);
bool AutoPatrol::sPos1Set = false;
bool AutoPatrol::sPos2Set = false;

void AutoPatrol::onEnable()
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (player == nullptr)
    {
        ChatUtils::displayClientMessage("§cPlayer not found!");
        setEnabled(false);
        return;
    }

    if (!sPos1Set || !sPos2Set)
    {
        ChatUtils::displayClientMessage("§cPlease set both pos1 and pos2 first!");
        ChatUtils::displayClientMessage("§7Use: .pos1 and .pos2");
        setEnabled(false);
        return;
    }

    mGoingToPos2 = true;
    mWaitTicks = 0;
    mCurrentPath.clear();
    mCurrentPathIndex = 0;

    // Рассчитываем начальный путь
    recalculatePath();

    ChatUtils::displayClientMessage("§aAutoPatrol enabled! Patrolling between pos1 and pos2...");

    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AutoPatrol::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &AutoPatrol::onRenderEvent>(this);
}

void AutoPatrol::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AutoPatrol::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &AutoPatrol::onRenderEvent>(this);
    
    mCurrentPath.clear();
    ChatUtils::displayClientMessage("§cAutoPatrol disabled.");
}

glm::vec3 AutoPatrol::getCurrentTarget() const
{
    return mGoingToPos2 ? sPos2 : sPos1;
}

void AutoPatrol::switchTarget()
{
    mGoingToPos2 = !mGoingToPos2;
    mWaitTicks = static_cast<int>(mWaitTime.mValue);
    recalculatePath();
}

void AutoPatrol::recalculatePath()
{
    mCurrentPath.clear();
    mCurrentPathIndex = 0;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player == nullptr) return;

    glm::vec3 startPos = *player->getPos();
    glm::vec3 targetPos = getCurrentTarget();

    if (mUsePathfinding.mValue)
    {
        // Используем pathfinding из AutoPath
        auto blockSource = ClientInstance::get()->getBlockSource();
        if (blockSource)
        {
            mCurrentPath = AutoPath::findFlightPathGlm(
                startPos, 
                targetPos, 
                blockSource, 
                mStopDistance.mValue, 
                true, 
                200, // timeout ms
                false // debug
            );
        }
    }
    
    // Если pathfinding не используется или не нашёл путь - просто прямая линия
    if (mCurrentPath.empty())
    {
        mCurrentPath.push_back(targetPos);
    }
}

void AutoPatrol::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (player == nullptr) return;

    // Ожидание на точке
    if (mWaitTicks > 0)
    {
        mWaitTicks--;
        return;
    }

    glm::vec3 playerPos = *player->getPos();
    glm::vec3 targetPos = getCurrentTarget();
    
    // Если используем pathfinding, следуем по пути
    glm::vec3 currentWaypoint = targetPos;
    if (!mCurrentPath.empty() && mCurrentPathIndex < mCurrentPath.size())
    {
        currentWaypoint = mCurrentPath[mCurrentPathIndex];
    }

    float distanceToWaypoint = glm::distance(playerPos, currentWaypoint);
    float distanceToFinalTarget = glm::distance(playerPos, targetPos);

    // Проверяем достижение промежуточной точки
    if (!mCurrentPath.empty() && mCurrentPathIndex < mCurrentPath.size())
    {
        if (distanceToWaypoint < mStopDistance.mValue)
        {
            mCurrentPathIndex++;
            if (mCurrentPathIndex >= mCurrentPath.size())
            {
                // Достигли конца пути
                if (mLoop.mValue)
                {
                    switchTarget();
                }
                else
                {
                    setEnabled(false);
                }
                return;
            }
            currentWaypoint = mCurrentPath[mCurrentPathIndex];
        }
    }
    // Проверяем достижение финальной цели (если без pathfinding)
    else if (distanceToFinalTarget < mStopDistance.mValue)
    {
        if (mLoop.mValue)
        {
            switchTarget();
        }
        else
        {
            setEnabled(false);
        }
        return;
    }

    // Вычисляем направление движения
    glm::vec3 direction = glm::normalize(currentWaypoint - playerPos);
    float speed = mSpeed.mValue * 0.1f; // Нормализуем скорость

    glm::vec3 velocity = direction * speed;

    if (mFlyMode.mValue)
    {
        // Режим полёта - двигаемся по всем осям
        glm::vec3 newPos = playerPos + velocity;
        player->setPosition(newPos);
    }
    else
    {
        // Режим ходьбы - только XZ, Y не трогаем (гравитация сама)
        auto stateVec = player->getStateVectorComponent();
        if (stateVec)
        {
            stateVec->mVelocity.x = velocity.x;
            stateVec->mVelocity.z = velocity.z;
            // Прыжок если нужно подняться
            if (currentWaypoint.y > playerPos.y + 0.5f)
            {
                if (player->isOnGround())
                {
                    stateVec->mVelocity.y = 0.42f; // Высота прыжка
                }
            }
        }
    }
}

void AutoPatrol::onRenderEvent(RenderEvent& event)
{
    if (!mRenderPath.mValue) return;
    if (!sPos1Set || !sPos2Set) return;

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player == nullptr) return;

    auto drawList = ImGui::GetBackgroundDrawList();

    // Рисуем точки pos1 и pos2
    auto drawPoint = [&](const glm::vec3& pos, ImU32 color, const std::string& label)
    {
        ImVec2 screenPos;
        if (RenderUtils::worldToScreen(pos, screenPos))
        {
            drawList->AddCircleFilled(screenPos, 8.0f, color);
            drawList->AddText(ImVec2(screenPos.x + 12, screenPos.y - 6), IM_COL32(255, 255, 255, 255), label.c_str());
        }

        // Рисуем 3D бокс
        AABB aabb = AABB(pos - glm::vec3(0.3f), pos + glm::vec3(0.3f));
        auto points = MathUtils::getImBoxPoints(aabb);
        if (!points.empty())
        {
            drawList->AddConvexPolyFilled(points.data(), points.size(), (color & 0x00FFFFFF) | 0x40000000);
            drawList->AddPolyline(points.data(), points.size(), color, 0, 2.f);
        }
    };

    // Pos1 - зелёный
    drawPoint(sPos1, IM_COL32(0, 255, 0, 255), "Pos1");
    
    // Pos2 - красный  
    drawPoint(sPos2, IM_COL32(255, 0, 0, 255), "Pos2");

    // Линия между точками
    ImVec2 screen1, screen2;
    if (RenderUtils::worldToScreen(sPos1, screen1) && RenderUtils::worldToScreen(sPos2, screen2))
    {
        drawList->AddLine(screen1, screen2, IM_COL32(255, 255, 0, 150), 2.0f);
    }

    // Рисуем текущий путь (если есть)
    if (!mCurrentPath.empty())
    {
        std::vector<ImVec2> pathPoints;
        for (const auto& pos : mCurrentPath)
        {
            ImVec2 point;
            if (RenderUtils::worldToScreen(pos, point))
            {
                pathPoints.push_back(point);
            }
        }

        // Соединяем точки пути
        for (size_t i = 0; i + 1 < pathPoints.size(); i++)
        {
            drawList->AddLine(pathPoints[i], pathPoints[i + 1], IM_COL32(0, 255, 255, 200), 3.0f);
        }

        // Рисуем текущую цель
        if (mCurrentPathIndex < mCurrentPath.size())
        {
            ImVec2 waypointScreen;
            if (RenderUtils::worldToScreen(mCurrentPath[mCurrentPathIndex], waypointScreen))
            {
                drawList->AddCircle(waypointScreen, 12.0f, IM_COL32(255, 165, 0, 255), 0, 3.0f);
            }
        }
    }

    // Показываем статус
    glm::vec3 playerPos = *player->getPos();
    float dist = glm::distance(playerPos, getCurrentTarget());
    std::string status = mGoingToPos2 ? "-> Pos2" : "-> Pos1";
    status += " (" + std::to_string(static_cast<int>(dist)) + "m)";
    
    if (mWaitTicks > 0)
    {
        status += " [Wait: " + std::to_string(mWaitTicks) + "]";
    }

    ImVec2 textPos(10, 100);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), ("AutoPatrol: " + status).c_str());
}