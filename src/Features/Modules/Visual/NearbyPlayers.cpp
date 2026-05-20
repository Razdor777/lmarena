#include "NearbyPlayers.hpp"
#include "HudEditor.hpp"
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>
#include <map>
#include <set>

static char sNearbyId[] = "NearbyPlayers";
class NearbyPlayersElement : public HudElement {
public:
  NearbyPlayersElement() : HudElement(sNearbyId) {
    mVisible = true;
    mAnchor = Anchor::TopLeft;
    mPos = {10.f, 220.f};
  }
};
static NearbyPlayersElement *gNearbyElem = nullptr;

// Функция для парсинга цветов Minecraft (премиум-рендер текста)
// Функция для парсинга цветов Minecraft (Идеальная работа с UTF-8 и кириллицей)
static void DrawMinecraftText(ImDrawList *dl, ImVec2 pos,
                              const std::string &text, float fontSize,
                              float alpha) {
  static std::map<char, ImColor> mcColors = {
      {'0', ImColor(0, 0, 0)},      {'1', ImColor(0, 0, 170)},
      {'2', ImColor(0, 170, 0)},    {'3', ImColor(0, 170, 170)},
      {'4', ImColor(170, 0, 0)},    {'5', ImColor(170, 0, 170)},
      {'6', ImColor(255, 170, 0)},  {'7', ImColor(170, 170, 170)},
      {'8', ImColor(85, 85, 85)},   {'9', ImColor(85, 85, 255)},
      {'a', ImColor(85, 255, 85)},  {'b', ImColor(85, 255, 255)},
      {'c', ImColor(255, 85, 85)},  {'d', ImColor(255, 85, 255)},
      {'e', ImColor(255, 255, 85)}, {'f', ImColor(255, 255, 255)},
      {'g', ImColor(221, 214, 5)},  {'r', ImColor(255, 255, 255)}};

  ImColor currentColor = ImColor(255, 255, 255, (int)(255 * alpha));
  float currentX = pos.x;
  std::string currentChunk = "";

  // Вспомогательная лямбда для отрисовки куска текста
  auto drawChunk = [&]() {
    if (!currentChunk.empty()) {
      dl->AddText(ImGui::GetFont(), fontSize, ImVec2(currentX, pos.y),
                  currentColor, currentChunk.c_str());
      currentX +=
          ImGui::GetFont()
              ->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, currentChunk.c_str())
              .x;
      currentChunk = "";
    }
  };

  for (size_t i = 0; i < text.length(); ++i) {
    // Проверка на параграф (в UTF-8 символ '§' - это два байта: 0xC2 0xA7)
    if ((unsigned char)text[i] == 0xC2 && i + 2 < text.length() &&
        (unsigned char)text[i + 1] == 0xA7) {
      drawChunk(); // Рисуем всё, что было до этого цвета
      char colorCode = tolower(text[i + 2]);
      if (mcColors.find(colorCode) != mcColors.end()) {
        currentColor = mcColors[colorCode];
        currentColor.Value.w = alpha; // сохраняем прозрачность
      }
      i += 2; // Пропускаем символ параграфа и сам код цвета
    }
    // Резервная проверка (если параграф вдруг пришел одним байтом)
    else if ((unsigned char)text[i] == 0xA7 && i + 1 < text.length()) {
      drawChunk();
      char colorCode = tolower(text[i + 1]);
      if (mcColors.find(colorCode) != mcColors.end()) {
        currentColor = mcColors[colorCode];
        currentColor.Value.w = alpha;
      }
      i += 1;
    } else {
      currentChunk += text[i]; // Добавляем букву в кусок
    }
  }
  drawChunk(); // Дорисовываем остаток
}

void NearbyPlayers::calculateHealths() {
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto actors = ActorUtils::getActorList(true, true);

    bool heal = 4000 <= NOW - mLastHealTime;
    if (heal) mLastHealTime = NOW;

    for (auto actor : actors) {
        if (!actor || actor == player) continue;

        try {
            if (!actor->getMobHurtTimeComponent()) continue;

            std::string rawName = ColorUtils::removeColorCodes(actor->getRawName());
            auto& info = mHealths[rawName];
            float absorption = actor->getAbsorption();
            int hurtTime = actor->getMobHurtTimeComponent()->mHurtTime;

            if (0 < hurtTime) {
                float damage = 0;
                if (absorption < info.lastAbsorption) {
                    if (0 < absorption) {
                        info.damage = abs(info.lastAbsorption - absorption);
                        damage = 0;
                    }
                    else if (0 < info.lastAbsorption) {
                        damage = abs(info.damage - info.lastAbsorption);
                    }
                }
                else if(hurtTime == 9)
                {
                    damage = info.damage;
                }

                if (absorption == 0 && 0 < damage) {
                    if (info.health - damage < 0) info.health = 0;
                    else info.health -= damage;
                }
            }

            if (heal) {
                if (info.health + 1 > 20) info.health = 20;
                else info.health++;
            }

            info.lastAbsorption = absorption;
        } catch (...) {
            continue;
        }
    }
}

void NearbyPlayers::onEnable() {
  mPlayers.clear();
  mKnownNames.clear();
  gFeatureManager->mDispatcher
      ->listen<BaseTickEvent, &NearbyPlayers::onBaseTickEvent>(this);
  gFeatureManager->mDispatcher
      ->listen<RenderEvent, &NearbyPlayers::onRenderEvent>(this);

  if (!gNearbyElem) {
    gNearbyElem = new NearbyPlayersElement();
    gNearbyElem->mSize = {200.f, 40.f};
    if (HudEditor::gInstance)
      HudEditor::gInstance->registerElement(gNearbyElem);
  }
  if (gNearbyElem)
    gNearbyElem->mVisible = true;

  mLastHealTime = NOW;
  mHealths.clear();
}

void NearbyPlayers::onDisable() {
  gFeatureManager->mDispatcher
      ->deafen<BaseTickEvent, &NearbyPlayers::onBaseTickEvent>(this);
  gFeatureManager->mDispatcher
      ->deafen<RenderEvent, &NearbyPlayers::onRenderEvent>(this);
  if (gNearbyElem)
    gNearbyElem->mVisible = false;
}

void NearbyPlayers::onBaseTickEvent(BaseTickEvent &event) {
  auto localPlayer = event.mActor;
  if (!localPlayer)
    return;

  calculateHealths(); // ← РАСЧЁТ HP

  glm::vec3 myPos = *localPlayer->getPos();
  float maxD = mMaxDist.mValue;
  auto actors = ActorUtils::getActorList(true, true);

  std::set<std::string> currentNames;
  std::vector<PlayerEntry> fresh;

  for (auto *actor : actors) {
    if (actor == localPlayer)
      continue;
    glm::vec3 p = *actor->getPos();
    float d = glm::distance(myPos, p);
    if (d > maxD)
      continue;

    std::string rawName = actor->getRawName(); // БОЛЬШЕ НЕ УДАЛЯЕМ ЦВЕТА!
    std::string cleanName =
        ColorUtils::removeColorCodes(rawName); // Чистое имя для логики
    currentNames.insert(cleanName);

    PlayerEntry pe;
    pe.name      = rawName;
    pe.dist      = d;
        
    // ← ИСПОЛЬЗУЕМ РАССЧИТАННОЕ HP
    float th = actor->getHealth();
    float tmh = actor->getMaxHealth();
    std::string targetName = actor->getNameTag();
    size_t nl = targetName.find('\n');
    if (nl != std::string::npos) targetName = targetName.substr(0, nl);

    bool tracked = false;
    if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
      tracked = true;
    } else if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
      tracked = true;
    }

    if (tracked) {
      pe.health = th;
      pe.maxHealth = tmh;
      mHealths[cleanName].health = th;
    } else {
      pe.health = mHealths.count(cleanName) ? mHealths[cleanName].health : actor->getHealth();
      pe.maxHealth = actor->getMaxHealth();
    }
    pe.introAnim = 0.f;
    pe.animHealth = pe.health; // Для плавной полоски
    pe.isNew = (mKnownNames.find(cleanName) == mKnownNames.end());
    pe.newPulse = pe.isNew ? 0.f : 1.f;

    for (auto &ex : mPlayers) {
      if (ColorUtils::removeColorCodes(ex.name) == cleanName) {
        pe.introAnim = ex.introAnim;
        pe.newPulse = ex.newPulse;
        pe.animHealth = ex.animHealth;
        pe.isNew = false;
        break;
      }
    }
    fresh.push_back(pe);
  }

  if (mSortDist.mValue) {
    std::sort(fresh.begin(), fresh.end(),
              [](const PlayerEntry &a, const PlayerEntry &b) {
                return a.dist < b.dist;
              });
  }

  int maxC = (int)mMaxCount.mValue;
  if ((int)fresh.size() > maxC)
    fresh.resize(maxC);

  mPlayers = std::move(fresh);
  mKnownNames = std::move(currentNames);
}

void NearbyPlayers::onRenderEvent(RenderEvent &event) {
  auto player = ClientInstance::get()->getLocalPlayer();
  if (!player)
    return;
  if (mPlayers.empty())
    return;

  auto dl = ImGui::GetBackgroundDrawList();
  float dt = ImGui::GetIO().DeltaTime;

  float scale = mScale.mValue;
  float fs = mFontSize.mValue * scale;
  float rowH = (fs + 14.f) * scale; // Сделали строки шире для полоски хп
  float width = 220.f * scale;
  float padL = 12.f * scale;

  int count = (int)mPlayers.size();
  float totalH = (float)count * rowH + 10.f * scale;

  ImVec2 base = gNearbyElem ? gNearbyElem->getPos() : ImVec2(10.f, 220.f);
  float x = base.x, y = base.y;

  // ПРЕМИУМ ФОН: Темный полупрозрачный градиент + тонкая обводка
  dl->AddRectFilled({x, y}, {x + width, y + totalH},
                    ImColor(15, 15, 20, (int)(220 * mOpacity.mValue)),
                    mRounding.mValue);
  dl->AddRect({x, y}, {x + width, y + totalH}, ImColor(255, 255, 255, 30),
              mRounding.mValue, 0, 1.5f);

  FontHelper::pushPrefFont(false);
  float ry = y + 5.f * scale;

  for (int i = 0; i < count; i++) {
    auto &pe = mPlayers[i];

    // Плавные анимации
    pe.introAnim = MathUtils::animate(1.f, pe.introAnim, dt * 15.f);
    pe.animHealth =
        MathUtils::animate(pe.health, pe.animHealth, dt * 10.f); // Плавное ХП
    float alpha = pe.introAnim;

    float textY = ry + 4.f * scale;

    // Рисуем имя с поддержкой § цветов
    DrawMinecraftText(dl, {x + padL, textY}, pe.name, fs, alpha);

    // Расстояние
    if (mShowDist.mValue) {
      char distBuf[16];
      snprintf(distBuf, sizeof(distBuf), "%.1fm", pe.dist);
      float distFs = fs * 0.85f;
      float dw = ImGui::GetFont()->CalcTextSizeA(distFs, FLT_MAX, 0, distBuf).x;

      ImColor dc = mColorDist.mValue
                       ? ImColor(255 - (int)(pe.dist * 2.5f),
                                 (int)(pe.dist * 2.5f), 50, (int)(255 * alpha))
                       : ColorUtils::getThemedColor((float)i * 35.f);
      dl->AddText(ImGui::GetFont(), distFs,
                  {x + width - dw - padL, textY + (fs - distFs) / 2.f}, dc,
                  distBuf);
    }

    // ПРЕМИУМ ПОЛОСКА ЗДОРОВЬЯ
    if (mShowHealth.mValue) {
      float hpBarY = ry + fs + 8.f * scale;
      float hpBarW = width - padL * 2.f;
      float hpBarH = 3.f * scale;
      float hpPercent = MathUtils::clamp(
          pe.animHealth / std::max(pe.maxHealth, 1.f), 0.f, 1.f);

      // Фон полоски
      dl->AddRectFilled({x + padL, hpBarY},
                        {x + padL + hpBarW, hpBarY + hpBarH},
                        ImColor(0, 0, 0, (int)(150 * alpha)), 2.f);

      // Заливка полоски (Градиент от красного к зеленому)
      ImColor hpColor = ImColor((int)(255 * (1.f - hpPercent)),
                                (int)(255 * hpPercent), 50, (int)(255 * alpha));
      dl->AddRectFilled({x + padL, hpBarY},
                        {x + padL + (hpBarW * hpPercent), hpBarY + hpBarH},
                        hpColor, 2.f);

      // Глоу-эффект для ХП (Свечение)
      hpColor.Value.w = alpha * 0.4f;
      dl->AddRectFilled(
          {x + padL, hpBarY - 1.f},
          {x + padL + (hpBarW * hpPercent), hpBarY + hpBarH + 1.f}, hpColor,
          3.f);
    }

    ry += rowH;
  }

  if (gNearbyElem)
    gNearbyElem->mSize = {width, totalH};
  ImGui::PopFont();
}