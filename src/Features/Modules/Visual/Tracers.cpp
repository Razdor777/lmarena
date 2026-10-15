//
// Tracers — перенос из combatant-client (visuals/Tracers.java).
//
// Отличия от оригинала (явно):
//   - стрелки рисуются процедурным треугольником: текстуры arrow1..4 из combatant
//     в Juzdex не переносились, выбора текстуры нет;
//   - «Inventory radius bonus» не переносился: в Juzdex нет проверки открытого
//     инвентаря в этом месте;
//   - линия не обрезается по краю экрана: если одна из точек за камерой, линия пропускается.
//

#include "Tracers.hpp"

#include <algorithm>
#include <cmath>

#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/MoveInputComponent.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace TracersImpl
{
    constexpr float kPi            = 3.14159265f;
    constexpr float kLineStartDist = 10.f;   // точка начала линии перед камерой
    constexpr float kBaseRadius    = 48.f;   // радиус круга стрелок (px)
    constexpr float kMoveRadius    = 1.5f;   // прибавка радиуса при движении
    constexpr float kMoveOffset    = 10.f;   // сдвиг круга при движении (px)
    constexpr float kArrowSize     = 10.f;
    constexpr float kArrowAlpha    = 235.f / 255.f;
    constexpr float kNearDist      = 6.f;    // ближе — минимальный радиус
    constexpr float kFarDist       = 60.f;   // дальше — полный радиус
    constexpr float kNearFactor    = 0.55f;
    constexpr float kFarFactor     = 1.f;
    constexpr float kLineAlpha     = 0.9f;
    constexpr float kLineWidth     = 1.5f;

    float radiusFactor(double dist)
    {
        if (dist <= kNearDist) return kNearFactor;
        if (dist >= kFarDist) return kFarFactor;
        const float t = static_cast<float>((dist - kNearDist) / (kFarDist - kNearDist));
        return kNearFactor + (kFarFactor - kNearFactor) * t;
    }

    // Экспоненциальное сглаживание, аналог AnimationUtility.fast из combatant.
    float approach(float cur, float target, float dt, float speed)
    {
        return cur + (target - cur) * (1.f - std::exp(-speed * dt));
    }

    ImU32 toU32(const ImVec4& c, float alpha)
    {
        return IM_COL32(static_cast<int>(std::clamp(c.x, 0.f, 1.f) * 255.f),
                        static_cast<int>(std::clamp(c.y, 0.f, 1.f) * 255.f),
                        static_cast<int>(std::clamp(c.z, 0.f, 1.f) * 255.f),
                        static_cast<int>(std::clamp(alpha, 0.f, 1.f) * 255.f));
    }

    // Треугольник-стрелка с вершиной, направленной от центра наружу под углом angle.
    void drawArrow(ImDrawList* dl, ImVec2 c, float angle, ImU32 col)
    {
        // Локальный +Y после поворота на (angle - 90°) смотрит вдоль angle.
        const float phi = angle - kPi * 0.5f;
        const float cs = std::cos(phi);
        const float sn = std::sin(phi);
        auto rot = [&](float x, float y) {
            return ImVec2(c.x + x * cs - y * sn, c.y + x * sn + y * cs);
        };
        const float h = kArrowSize * 0.5f;
        ImVec2 pts[3] = { rot(0.f, h), rot(-h, -h), rot(h, -h) };
        dl->AddConvexPolyFilled(pts, 3, col);
    }
}

using namespace TracersImpl;

void Tracers::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Tracers::onRenderEvent>(this);
}

void Tracers::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Tracers::onRenderEvent>(this);
}

void Tracers::onRenderEvent(RenderEvent& event)
{
    auto* localPlayer = ClientInstance::get()->getLocalPlayer();
    if (!localPlayer || !ClientInstance::get()->getLevelRenderer()) return;

    const glm::vec3 camPos = RenderUtils::transform.mOrigin;
    const float dt = ImGui::GetIO().DeltaTime;

    // Направление взгляда камеры: yaw — из головы, pitch — из тела.
    float yawDeg = 0.f;
    float pitchDeg = 0.f;
    if (auto* head = localPlayer->getActorHeadRotationComponent()) yawDeg = head->mHeadRot;
    if (auto* rot = localPlayer->getActorRotationComponent()) pitchDeg = rot->mPitch;

    if (mMode.mValue == Mode::Lines)
    {
        const float yaw = yawDeg * kPi / 180.f;
        const float pitch = pitchDeg * kPi / 180.f;
        const glm::vec3 look(-std::sin(yaw) * std::cos(pitch),
                             -std::sin(pitch),
                             std::cos(yaw) * std::cos(pitch));
        drawLines(localPlayer, camPos, look);
    }
    else
    {
        // Стрелки имеют смысл только от первого лица (как в combatant).
        if (ClientInstance::get()->getOptions()->mThirdPerson->value != 0) return;
        drawArrows(localPlayer, camPos, yawDeg, dt);
    }
}

void Tracers::drawLines(Actor* localPlayer, const glm::vec3& camPos, const glm::vec3& look)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    ImVec2 start;
    if (!RenderUtils::worldToScreen(camPos + look * kLineStartDist, start)) return;

    for (Actor* actor : ActorUtils::getActorList(true, false))
    {
        if (actor == localPlayer) continue;

        const bool friendly = gFriendManager->isFriend(actor);
        if (friendly && !mTraceFriends.mValue) continue;

        auto* pos = actor->getPos();
        if (!pos) continue;

        // getPos() — на высоте глаз; линия идёт к ногам, как getPosition() в combatant.
        const glm::vec3 feet = *pos - PLAYER_HEIGHT_VEC;

        ImVec2 end;
        if (!RenderUtils::worldToScreen(feet, end)) continue;

        const ImVec4 color = friendly ? ImVec4(0.f, 1.f, 0.f, 1.f) : ColorUtils::getThemedColor(0).Value;
        dl->AddLine(start, end, toU32(color, kLineAlpha), kLineWidth);
    }
}

void Tracers::drawArrows(Actor* localPlayer, const glm::vec3& camPos, float yawDeg, float dt)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 center = { display.x * 0.5f, display.y * 0.5f };

    glm::vec2 move(0.f, 0.f);
    if (mMoveRadiusBonus.mValue)
    {
        if (auto* input = localPlayer->getMoveInputComponent()) move = input->mMoveVector;
    }
    const bool moving = move.x != 0.f || move.y != 0.f;

    // Сдвиг круга при движении и сглаженная камера.
    const float moveScale = mMoveRadiusBonus.mValue ? kMoveOffset : 0.f;
    mAnimYaw = approach(mAnimYaw, move.x * moveScale, dt, 5.f);
    mAnimPitch = approach(mAnimPitch, move.y * moveScale, dt, 5.f);
    mYaw = approach(mYaw, yawDeg, dt, 10.f);

    float size = kBaseRadius;
    if (mMoveRadiusBonus.mValue && moving) size += kMoveRadius;
    mAnimStep = approach(mAnimStep, size, dt, 6.f);

    const float yawRad = mYaw * kPi / 180.f;
    const float cosY = std::cos(yawRad);
    const float sinY = std::sin(yawRad);

    for (Actor* actor : ActorUtils::getActorList(true, false))
    {
        if (actor == localPlayer) continue;

        const bool friendly = gFriendManager->isFriend(actor);
        if (friendly && !mTraceFriends.mValue) continue;

        auto* pos = actor->getPos();
        if (!pos) continue;
        const glm::vec3 feet = *pos - PLAYER_HEIGHT_VEC;

        // Угол цели относительно взгляда камеры (формула из Tracers.java).
        const float relX = feet.x - camPos.x;
        const float relZ = feet.z - camPos.z;
        const float rotY = -(relZ * cosY - relX * sinY);
        const float rotX = -(relX * cosY + relZ * sinY);
        const float angle = std::atan2(rotY, rotX);

        const double dist = glm::distance(camPos, feet);
        const double radius = mAnimStep * radiusFactor(dist);

        const ImVec2 p = { static_cast<float>(radius * std::cos(angle)) + center.x + mAnimYaw,
                           static_cast<float>(radius * std::sin(angle)) + center.y + mAnimPitch };

        const ImVec4 color = friendly ? ImVec4(0.f, 1.f, 0.f, 1.f) : ColorUtils::getThemedColor(0).Value;
        drawArrow(dl, p, angle, toU32(color, kArrowAlpha));
    }
}
