#include "EnemyIndicator.hpp"
#include "HudEditor.hpp"
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <glm/glm.hpp>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

static char sEnemyIndId[] = "EnemyIndicator";
class EnemyIndElement : public HudElement {
public:
    EnemyIndElement() : HudElement(sEnemyIndId) {
        mVisible = true; mCentered = true; mAnchor = Anchor::Middle;
    }
};
static EnemyIndElement* gEnemyElem = nullptr;

void EnemyIndicator::onEnable() {
    gFeatureManager->mDispatcher->listen<RenderEvent, &EnemyIndicator::onRenderEvent>(this);
    if (!gEnemyElem) {
        gEnemyElem = new EnemyIndElement();
        gEnemyElem->mSize = {0.f, 0.f};
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gEnemyElem);
    }
    if (gEnemyElem) gEnemyElem->mVisible = true;
}

void EnemyIndicator::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &EnemyIndicator::onRenderEvent>(this);
    if (gEnemyElem) gEnemyElem->mVisible = false;
}

// ПРЕМИУМ-СТРЕЛКА: Красивый изогнутый шеврон со свечением, направленный ровно от центра
// ПРЕМИУМ-СТРЕЛКА: Смотрит ОТ ЦЕНТРА НАРУЖУ на противника
static void drawPremiumChevron(ImDrawList* dl, float cx, float cy, float angle, float size, ImColor color) {
    float cosA = cosf(angle), sinA = sinf(angle);
    auto rot = [&](float lx, float ly) -> ImVec2 {
        return {cx + lx * cosA - ly * sinA, cy + lx * sinA + ly * cosA};
    };

    // ИЗМЕНЕНО: Теперь острие смотрит наружу (в минус), а задняя часть ближе к центру (в плюс)
    ImVec2 tip = rot(0.f, -size);                  
    ImVec2 backLeft = rot(-size * 0.7f, size * 0.8f);
    ImVec2 backRight = rot(size * 0.7f, size * 0.8f);
    ImVec2 innerCenter = rot(0.f, size * 0.3f);  

    // 1. Создаем эффект неонового свечения (Glow)
    ImColor glow = color;
    glow.Value.w *= 0.25f; // Полупрозрачность для свечения
    dl->AddQuad(tip, backRight, innerCenter, backLeft, glow, 5.0f);
    dl->AddQuad(tip, backRight, innerCenter, backLeft, glow, 9.0f);

    // 2. Основная заливка
    dl->AddQuadFilled(tip, backRight, innerCenter, backLeft, color);

    // 3. Жесткая белая/светлая обводка для четкости
    ImColor outline = ImColor(255, 255, 255, (int)(180 * color.Value.w));
    dl->AddQuad(tip, backRight, innerCenter, backLeft, outline, 1.5f);
}

void EnemyIndicator::onRenderEvent(RenderEvent& event) {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto dl = ImGui::GetBackgroundDrawList();
    float now = (float)ImGui::GetTime();

    ImVec2 screenCenter;
    if (gEnemyElem && gEnemyElem->mVisible) {
        screenCenter = gEnemyElem->getPos();
        if (screenCenter.x == 0.f && screenCenter.y == 0.f) {
            auto disp = ImGui::GetIO().DisplaySize;
            screenCenter = {disp.x * 0.5f, disp.y * 0.5f};
        }
    } else {
        auto disp = ImGui::GetIO().DisplaySize;
        screenCenter = {disp.x * 0.5f, disp.y * 0.5f};
    }

    glm::vec3 myPos = *player->getPos();
    
    // Получаем поворот камеры/игрока
    float myYaw = player->getActorRotationComponent()->mYaw;

    float maxRange = mRange.mValue;
    float ringR    = mRadius.mValue;
    float arrowSz  = mArrowSize.mValue;
    int   maxN     = (int)mMaxArrows.mValue;
    float opacity  = mOpacity.mValue;

    struct EnemyData { Actor* a; float dist; float screenAngle; };
    std::vector<EnemyData> enemies;

    for (auto* actor : ActorUtils::getActorList(true, true)) {
        if (actor == player) continue;
        glm::vec3 p = *actor->getPos();
        float d = glm::distance(myPos, p);
        if (d > maxRange) continue;

        // ПРАВИЛЬНАЯ МАТЕМАТИКА НАПРАВЛЕНИЯ
        // Вычисляем угол между нами и врагом в мире
        float deltaX = p.x - myPos.x;
        float deltaZ = p.z - myPos.z;
        
        // Atan2 в майне. Обрати внимание, Z - это как бы Y на плоскости
        float worldYaw = atan2f(deltaZ, deltaX) * (180.f / M_PI);
        
        // Корректируем относительно того, куда смотрит игрок
        // Майновский Yaw смещен на 90 градусов, учитываем это
        float relativeYaw = worldYaw - myYaw - 90.f;
        
        // Переводим в радианы для отрисовки (0 - это верх экрана)
        float screenAngle = glm::radians(relativeYaw);

        enemies.push_back({actor, d, screenAngle});
    }

    std::sort(enemies.begin(), enemies.end(), [](const EnemyData& a, const EnemyData& b){
        return a.dist < b.dist;
    });
    if ((int)enemies.size() > maxN) enemies.resize(maxN);
    if (enemies.empty()) return;

    int i = 0;
    for (auto& en : enemies) {
        float angle = en.screenAngle;
        
        // Круговая орбита вокруг центра экрана
        // sin(angle) для оси X, -cos(angle) для оси Y (так как Y в играх идет вниз)
        float ax = screenCenter.x + sinf(angle) * ringR;
        float ay = screenCenter.y - cosf(angle) * ringR;

        ImColor arrowCol;
        if (mColorDist.mValue) {
            float t = std::clamp(en.dist / maxRange, 0.f, 1.f);
            // Плавный градиент: Красный (близко) -> Желтый -> Зеленый (далеко)
            arrowCol = ImColor((int)(255 * (1.f - t)), (int)(255 * t), 50, 255);
        } else {
            arrowCol = ColorUtils::getThemedColor((float)i * 55.f);
        }

        // Эффект сердцебиения/пульсации, если враг подошел вплотную
        float finalAlpha = opacity;
        if (mPulseClose.mValue && en.dist < 10.f) {
            float pulse = 0.7f + 0.3f * sinf(now * 12.f); // Быстро пульсирует
            finalAlpha *= pulse;
            arrowSz *= pulse; // Стрелка немного увеличивается
        }
        arrowCol.Value.w = finalAlpha;

        // Рисуем крутой шеврон
        drawPremiumChevron(dl, ax, ay, angle, arrowSz, arrowCol);
        i++;
    }
}