//
// Backtrack (visual) — см. комментарий в Backtrack.hpp.
//

#include "Backtrack.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>

namespace
{
    inline uint64_t nowMs()
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    }

    ImU32 withAlpha(const ImColor& c, float alpha)
    {
        const float a = std::clamp(alpha, 0.f, 1.f);
        return IM_COL32(static_cast<int>(c.Value.x * 255.f),
                        static_cast<int>(c.Value.y * 255.f),
                        static_cast<int>(c.Value.z * 255.f),
                        static_cast<int>(a * 255.f));
    }
}

void Backtrack::onEnable()
{
    sHistory.clear();
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &Backtrack::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,    &Backtrack::onRenderEvent>(this);
}

void Backtrack::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &Backtrack::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &Backtrack::onRenderEvent>(this);
    sHistory.clear();
}

// ==================== СБОР ИСТОРИИ ====================

void Backtrack::onBaseTickEvent(BaseTickEvent& event)
{
    auto* ci = ClientInstance::get();
    if (!ci) return;

    auto* localPlayer = ci->getLocalPlayer();
    if (!localPlayer) return;

    // BaseTickEvent приходит только для своей модели (см. BaseTickHook) —
    // лишняя проверка стоит один указатель и защищает от будущих изменений.
    if (event.mActor && event.mActor != localPlayer) return;

    const int64_t now = static_cast<int64_t>(nowMs());

    std::unordered_set<int64_t> seen;
    std::vector<Actor*> actors;
    try {
        actors = ActorUtils::getActorList(true, false);   // только игроки
    } catch (...) {
        return;
    }

    for (Actor* actor : actors)
    {
        if (!actor || actor == localPlayer) continue;

        try {
            if (actor->isDead()) continue;

            auto* renderPos = actor->getRenderPositionComponent();
            auto* shape     = actor->getAABBShapeComponent();
            if (!renderPos || !shape) continue;

            if (localPlayer->distanceTo(actor) > mRange.mValue) continue;

            Sample s;
            s.pos    = renderPos->mPosition;
            if (actor->isPlayer()) s.pos.y -= PLAYER_HEIGHT;   // RenderPosition — на высоте глаз
            s.width  = shape->mWidth;
            s.height = shape->mHeight;
            s.t      = now;

            if (!std::isfinite(s.pos.x) || !std::isfinite(s.pos.y) || !std::isfinite(s.pos.z))
                continue;
            if (!std::isfinite(s.width) || !std::isfinite(s.height) ||
                s.width <= 0.f || s.height <= 0.f)
                continue;

            const int64_t id = actor->getRuntimeID();
            auto& history = sHistory[id];

            // Та же позиция, что и в прошлом тике — не раздуваем историю.
            if (!history.empty() &&
                glm::distance(history.back().pos, s.pos) < 0.001f &&
                now - history.back().t < 200)
            {
                seen.insert(id);
                continue;
            }

            history.push_back(s);
            while (!history.empty() && now - history.front().t > kMaxHistoryMs)
                history.pop_front();
            while (history.size() > 128)
                history.pop_front();

            seen.insert(id);
        } catch (...) {
            continue;
        }
    }

    // Мёртвые/ушедшие игроки: как только их последняя запись устарела,
    // выкидываем ключ целиком, иначе карта растёт всю сессию.
    for (auto it = sHistory.begin(); it != sHistory.end(); )
    {
        if (seen.contains(it->first)) { ++it; continue; }

        if (it->second.empty() || now - it->second.back().t > kMaxHistoryMs)
            it = sHistory.erase(it);
        else
            ++it;
    }
}

// ==================== ОТРИСОВКА ====================

const Backtrack::Sample* Backtrack::sampleAt(const std::deque<Sample>& history, int64_t wantedT)
{
    if (history.empty()) return nullptr;

    // Ищем последнюю запись не новее wantedT. История короткая (≤128), поэтому
    // линейный проход с конца дешевле любой возни с итераторами.
    for (auto it = history.rbegin(); it != history.rend(); ++it)
    {
        if (it->t <= wantedT) return &*it;
    }

    // Отложенной позиции ещё нет (только зашли на сервер) — берём самую старую.
    return &history.front();
}

void Backtrack::drawBox(ImDrawList* dl, const Sample& s, ImU32 col, float thickness)
{
    const float halfW = s.width * 0.5f;

    AABB box;
    box.mMin = s.pos - glm::vec3(halfW, 0.f,     halfW);
    box.mMax = s.pos + glm::vec3(halfW, s.height, halfW);

    const std::vector<ImVec2> pts = MathUtils::getImBoxPoints(box);
    if (pts.size() < 3) return;   // часть углов за камерой — не рисуем кривой полигон

    dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), col, ImDrawFlags_Closed, thickness);
}

void Backtrack::onRenderEvent(RenderEvent& event)
{
    if (sHistory.empty()) return;
    if (!ImGui::GetCurrentContext()) return;

    auto* ci = ClientInstance::get();
    if (!ci || !ci->getLocalPlayer()) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;

    const int64_t now       = static_cast<int64_t>(nowMs());
    const int64_t delayMs   = static_cast<int64_t>(mDelay.mValue);
    const int64_t trailMs   = static_cast<int64_t>(mTrailMs.mValue);
    const int64_t wantedT   = now - delayMs;

    const ImColor theme = ColorUtils::getThemedColor(0);

    for (const auto& [id, history] : sHistory)
    {
        if (history.empty()) continue;

        const Sample* ghost = sampleAt(history, wantedT);
        if (!ghost) continue;

        try
        {
            // След: боксы по истории, от призрака назад, с затуханием.
            if (mTrail.mValue && trailMs > 0)
            {
                const int64_t step = 100;   // ~5 боксов на 500 мс, больше — визуальный шум
                int64_t lastDrawnT = ghost->t;

                for (auto it = history.rbegin(); it != history.rend(); ++it)
                {
                    if (it->t > ghost->t) continue;                 // новее призрака не рисуем
                    if (it->t < ghost->t - trailMs) break;          // хвост закончился
                    if (lastDrawnT - it->t < step) continue;        // слишком близко к предыдущему

                    lastDrawnT = it->t;
                    const float k = 1.f - static_cast<float>(ghost->t - it->t) / static_cast<float>(trailMs);
                    drawBox(dl, *it, withAlpha(theme, 0.45f * std::clamp(k, 0.f, 1.f)), 1.f);
                }
            }

            // Сам призрак — ярко и толще, чтобы читался на фоне следа.
            drawBox(dl, *ghost, withAlpha(theme, 0.9f), 2.f);

            // Линия «призрак → где игрок сейчас (по последней записи)».
            if (mLine.mValue && !history.empty())
            {
                const Sample& latest = history.back();

                ImVec2 a, b;
                const glm::vec3 ghostTop (ghost->pos.x,  ghost->pos.y  + ghost->height, ghost->pos.z);
                const glm::vec3 latestTop(latest.pos.x, latest.pos.y + latest.height, latest.pos.z);

                if (RenderUtils::worldToScreen(ghostTop, a) &&
                    RenderUtils::worldToScreen(latestTop, b))
                {
                    dl->AddLine(a, b, withAlpha(theme, 0.35f), 1.f);
                }
            }
        }
        catch (...) {
            // Ничего не рисуем, история останется — один пропущенный кадр не беда.
            continue;
        }
    }
}
