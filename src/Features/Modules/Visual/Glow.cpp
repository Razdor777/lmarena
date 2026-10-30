//
// Glow — см. комментарий в Glow.hpp.
//

#include "Glow.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

namespace
{
    ImU32 withAlpha(const ColorSetting& setting, float alpha)
    {
        const float a = std::clamp(alpha * setting.mValue[3], 0.f, 1.f);
        return IM_COL32(static_cast<int>(setting.mValue[0] * 255.f),
                        static_cast<int>(setting.mValue[1] * 255.f),
                        static_cast<int>(setting.mValue[2] * 255.f),
                        static_cast<int>(a * 255.f));
    }
}

void Glow::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Glow::onRenderEvent>(this);
}

void Glow::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Glow::onRenderEvent>(this);
}

void Glow::onRenderEvent(RenderEvent& event)
{
    if (!ImGui::GetCurrentContext()) return;

    auto* ci = ClientInstance::get();
    if (!ci) return;

    auto* localPlayer = ci->getLocalPlayer();
    if (!localPlayer) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    std::vector<Actor*> actors;
    try {
        actors = ActorUtils::getActorList(true, false);   // только игроки
    } catch (...) {
        return;
    }

    // Проходы от самого широкого и прозрачного к тонкому и плотному: так край
    // читается как рассеянный свет, а не как ещё один ESP-бокс.
    const int passes = 6;

    for (Actor* actor : actors)
    {
        if (!actor || actor == localPlayer) continue;

        try {
            if (actor->isDead()) continue;
            if (localPlayer->distanceTo(actor) > mRange.mValue) continue;

            // Тот же путь, что ESP: getAABB() сам ставит коробку на «ноги»,
            // берёт интерполированную позицию и совпадает с остальными модулями.
            AABB box = actor->getAABB();

            if (!std::isfinite(box.mMin.x) || !std::isfinite(box.mMin.y) || !std::isfinite(box.mMin.z) ||
                !std::isfinite(box.mMax.x) || !std::isfinite(box.mMax.y) || !std::isfinite(box.mMax.z))
                continue;

            const std::vector<ImVec2> pts = MathUtils::getImBoxPoints(box);
            if (pts.size() < 3) continue;   // часть углов за камерой — полигон был бы кривой

            if (mFill.mValue)
            {
                dl->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()),
                                        withAlpha(mColor, 0.10f));
            }

            for (int i = 0; i < passes; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(passes - 1); // 0..1

                // Толщина растёт наружу, прозрачность падает квадратично —
                // свечение затухает без резкой границы.
                const float thickness = 1.f + t * mSpread.mValue;
                const float alpha     = 0.55f * (1.f - t) * (1.f - t);

                if (alpha <= 0.01f) continue;

                dl->AddPolyline(pts.data(), static_cast<int>(pts.size()),
                                withAlpha(mColor, alpha), ImDrawFlags_Closed, thickness);
            }
        } catch (...) {
            continue;
        }
    }
}
