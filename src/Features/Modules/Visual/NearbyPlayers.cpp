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

// Проверяет содержит ли строка кириллические символы (U+0400–U+04FF)
static bool hasCyrillic(const std::string &text) {
  for (size_t i = 0; i < text.length();) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (c < 0x80) { i++; continue; }

    uint32_t cp = 0;
    if (c >= 0xC0 && c < 0xE0 && i + 1 < text.length()) {
      cp = ((uint32_t)(c & 0x1F) << 6) | (text[i + 1] & 0x3F);
      i += 2;
    } else if (c >= 0xE0 && c < 0xF0 && i + 2 < text.length()) {
      cp = ((uint32_t)(c & 0x0F) << 12) |
           ((uint32_t)(text[i + 1] & 0x3F) << 6) |
           (text[i + 2] & 0x3F);
      i += 3;
    } else if (c >= 0xF0 && i + 3 < text.length()) {
      cp = ((uint32_t)(c & 0x07) << 18) |
           ((uint32_t)(text[i + 1] & 0x3F) << 12) |
           ((uint32_t)(text[i + 2] & 0x3F) << 6) |
           (text[i + 3] & 0x3F);
      i += 4;
    } else {
      i++;
      continue;
    }
    if (cp >= 0x0400 && cp <= 0x04FF) return true;
  }
  return false;
}

// Получить шрифт с поддержкой кириллицы
static ImFont *getCyrillicFont() {
  // Сначала пробуем текущий шрифт — если он поддерживает кириллицу, ок
  // Но лучше сразу вернуть заведомо поддерживающий
  auto it = FontHelper::Fonts.find("open_sans");
  if (it != FontHelper::Fonts.end() && it->second) return it->second;
  it = FontHelper::Fonts.find("product_sans");
  if (it != FontHelper::Fonts.end() && it->second) return it->second;
  it = FontHelper::Fonts.find("comfortaa");
  if (it != FontHelper::Fonts.end() && it->second) return it->second;
  return ImGui::GetFont(); // последний fallback
}

// Улучшенный рендер текста с поддержкой § цветов и UTF-8
static void DrawMinecraftText(ImDrawList *dl, ImVec2 pos,
                              const std::string &text, float fontSize,
                              float alpha, ImColor defaultColor = ImColor(255,255,255)) {
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

  ImFont* font = FontHelper::getFont(false, false, false);
  if (!font) font = ImGui::GetFont();

  ImColor currentColor = defaultColor;
  currentColor.Value.w = alpha;
  float currentX = pos.x;
  std::string currentChunk = "";

  auto drawChunk = [&]() {
    if (!currentChunk.empty()) {
      dl->AddText(font, fontSize, ImVec2(currentX, pos.y),
                  currentColor, currentChunk.c_str());
      currentX +=
          font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, currentChunk.c_str()).x;
      currentChunk = "";
    }
  };

  for (size_t i = 0; i < text.length();) {
    if ((unsigned char)text[i] == 0xC2 && i + 2 < text.length() &&
        (unsigned char)text[i + 1] == 0xA7) {
      drawChunk();
      char colorCode = tolower(text[i + 2]);
      if (mcColors.find(colorCode) != mcColors.end()) {
        currentColor = mcColors[colorCode];
        currentColor.Value.w = alpha;
      }
      i += 3;
    }
    else if ((unsigned char)text[i] == 0xA7 && i + 1 < text.length()) {
      drawChunk();
      char colorCode = tolower(text[i + 1]);
      if (mcColors.find(colorCode) != mcColors.end()) {
        currentColor = mcColors[colorCode];
        currentColor.Value.w = alpha;
      }
      i += 2;
    } else {
      size_t len = 1;
      unsigned char uc = static_cast<unsigned char>(text[i]);
      if (uc >= 0xC0 && uc < 0xE0) len = 2;
      else if (uc >= 0xE0 && uc < 0xF0) len = 3;
      else if (uc >= 0xF0 && uc < 0xF8) len = 4;
      if (i + len > text.length()) len = text.length() - i;
      currentChunk += text.substr(i, len);
      i += len;
    }
  }
  drawChunk();
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

// TargetHUD-style health calculation
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

void NearbyPlayers::onBaseTickEvent(BaseTickEvent &event) {
  auto localPlayer = event.mActor;
  if (!localPlayer)
    return;

  calculateHealths();

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

    std::string rawName = actor->getRawName();
    std::string cleanName = ColorUtils::removeColorCodes(rawName);
    currentNames.insert(cleanName);

    PlayerEntry pe;
    pe.name = rawName;
    pe.dist = d;

    // HP calculation like TargetHUD
    float health = actor->getHealth();
    float maxHealth = actor->getMaxHealth();
    
    if (actor->isPlayer()) {
        std::string targetName = actor->getNameTag();
        size_t nl = targetName.find('\n');
        if (nl != std::string::npos) targetName = targetName.substr(0, nl);
        
        float th = health, tmh = maxHealth;
        bool tracked = false;
        if (HealthTracker::getInstance().getHealth(targetName, th, tmh)) {
            health = th;
            maxHealth = tmh;
            tracked = true;
        } else {
            if (HealthTracker::getInstance().getHealth(cleanName, th, tmh)) {
                health = th;
                maxHealth = tmh;
                tracked = true;
            }
        }
        
        if (!tracked && mHealths.count(cleanName)) {
            health = mHealths[cleanName].health;
        }
    }
    
    pe.health = health;
    pe.maxHealth = maxHealth;
    pe.introAnim = 0.f;
    pe.animHealth = health;
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
  float rowH = (fs + 18.f) * scale;
  float width = 240.f * scale;
  float padL = 14.f * scale;

  int count = (int)mPlayers.size();
  float totalH = (float)count * rowH + 12.f * scale;

  ImVec2 base = gNearbyElem ? gNearbyElem->getPos() : ImVec2(10.f, 220.f);
  float x = base.x, y = base.y;

  // Modern glassmorphism background with blur
  if (mGlassStyle.mValue) {
    ImRenderUtils::addBlur(ImVec4(x - 2, y - 2, x + width + 2, y + totalH + 2), 3 * scale, 8);
  }
  
  // Background
  dl->AddRectFilled({x, y}, {x + width, y + totalH},
                    ImColor(12, 12, 16, (int)(230 * mOpacity.mValue)),
                    mRounding.mValue);
  
  // Top accent line
  ImColor accent = ColorUtils::getThemedColor(0);
  accent.Value.w = 0.8f;
  dl->AddRectFilled({x, y}, {x + width, y + 2.f * scale}, accent, mRounding.mValue, ImDrawFlags_RoundCornersTop);
  
  // Border
  dl->AddRect({x, y}, {x + width, y + totalH}, ImColor(255, 255, 255, 25),
              mRounding.mValue, 0, 1.f);

  FontHelper::pushPrefFont(false);
  float ry = y + 8.f * scale;

  for (int i = 0; i < count; i++) {
    auto &pe = mPlayers[i];

    pe.introAnim = MathUtils::animate(1.f, pe.introAnim, dt * 12.f);
    pe.animHealth = MathUtils::animate(pe.health, pe.animHealth, dt * 8.f);
    
    if (mPulseNew.mValue && pe.isNew) {
        pe.newPulse = MathUtils::animate(1.f, pe.newPulse, dt * 4.f);
    }
    
    float alpha = pe.introAnim;
    float textY = ry + 5.f * scale;

    // New player highlight
    if (mPulseNew.mValue && !pe.isNew && pe.newPulse < 1.f) {
        float pulseAlpha = (1.f - pe.newPulse) * 0.15f;
        dl->AddRectFilled({x + 2, ry}, {x + width - 2, ry + rowH - 2}, 
                          ImColor(255, 255, 255, (int)(255 * pulseAlpha)), 4.f);
    }

    // Name with color support
    DrawMinecraftText(dl, {x + padL, textY}, pe.name, fs, alpha);

    // Distance
    if (mShowDist.mValue) {
      char distBuf[16];
      snprintf(distBuf, sizeof(distBuf), "%.1fm", pe.dist);
      float distFs = fs * 0.8f;
      float dw = ImGui::GetFont()->CalcTextSizeA(distFs, FLT_MAX, 0, distBuf).x;

      ImColor dc;
      if (mColorDist.mValue) {
        float t = std::clamp(pe.dist / mMaxDist.mValue, 0.f, 1.f);
        dc = ImColor((int)(255 * t), (int)(255 * (1.f - t)), 80, (int)(255 * alpha));
      } else {
        dc = ColorUtils::getThemedColor((float)i * 35.f);
        dc.Value.w = alpha;
      }
      dl->AddText(ImGui::GetFont(), distFs,
                  {x + width - dw - padL, textY + (fs - distFs) * 0.3f}, dc,
                  distBuf);
    }

    // Health bar
    if (mShowHealth.mValue) {
      float hpBarY = ry + fs + 8.f * scale;
      float hpBarW = width - padL * 2.f;
      float hpBarH = 4.f * scale;
      float hpPercent = MathUtils::clamp(
          pe.animHealth / std::max(pe.maxHealth, 1.f), 0.f, 1.f);

      // Background
      dl->AddRectFilled({x + padL, hpBarY},
                        {x + padL + hpBarW, hpBarY + hpBarH},
                        ImColor(30, 30, 35, (int)(200 * alpha)), hpBarH);

      // Fill color based on health
      ImColor hpColor;
      if (hpPercent > 0.6f) hpColor = ImColor(80, 220, 120, (int)(255 * alpha));
      else if (hpPercent > 0.3f) hpColor = ImColor(255, 200, 60, (int)(255 * alpha));
      else hpColor = ImColor(255, 70, 70, (int)(255 * alpha));
      
      dl->AddRectFilled({x + padL, hpBarY},
                        {x + padL + (hpBarW * hpPercent), hpBarY + hpBarH},
                        hpColor, hpBarH);
      
      // HP text
      char hpBuf[16];
      snprintf(hpBuf, sizeof(hpBuf), "%.0f", pe.animHealth);
      float hpTextW = ImGui::GetFont()->CalcTextSizeA(fs * 0.7f, FLT_MAX, 0, hpBuf).x;
      dl->AddText(ImGui::GetFont(), fs * 0.7f, 
                  {x + padL + hpBarW * hpPercent - hpTextW - 2.f, hpBarY - fs * 0.5f},
                  ImColor(255, 255, 255, (int)(200 * alpha)), hpBuf);
    }

    // Separator line
    if (i < count - 1) {
        dl->AddLine({x + padL, ry + rowH - 1}, {x + width - padL, ry + rowH - 1}, 
                    ImColor(255, 255, 255, 15), 1.f);
    }

    ry += rowH;
  }

  if (gNearbyElem)
    gNearbyElem->mSize = {width, totalH};
  ImGui::PopFont();
}
