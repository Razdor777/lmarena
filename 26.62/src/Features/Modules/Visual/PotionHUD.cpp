#include "PotionHUD.hpp"
#include "HudEditor.hpp"
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/ItemTextures.hpp>

static char sPotionHUDId[] = "PotionHUD";
class PotionHudElement : public HudElement {
public:
    PotionHudElement() : HudElement(sPotionHUDId) {
        mVisible = true; mAnchor = Anchor::TopRight;
        mPos = {-230.f, 80.f};
    }
};
static PotionHudElement* gPotionElem = nullptr;

bool PotionHUD::isNegativeEffect(EffectType t)
{
    switch(t) {
    case EffectType::Slowness: case EffectType::MiningFatigue:
    case EffectType::InstantDamage: case EffectType::Nausea:
    case EffectType::Blindness: case EffectType::Hunger:
    case EffectType::Weakness: case EffectType::Poison:
    case EffectType::Wither: case EffectType::Levitation:
    case EffectType::FatalPoison: case EffectType::BadOmen:
    case EffectType::Darkness:
        return true;
    default: return false;
    }
}

ImColor PotionHUD::effectColor(EffectType t)
{
    switch(t) {
    case EffectType::Speed:          return ImColor(127,200,255);
    case EffectType::Slowness:       return ImColor(130,130,180);
    case EffectType::Haste:          return ImColor(255,220,100);
    case EffectType::MiningFatigue:  return ImColor(100,100,80);
    case EffectType::Strength:       return ImColor(220,60,60);
    case EffectType::InstantHealth:  return ImColor(240,100,180);
    case EffectType::InstantDamage:  return ImColor(180,0,80);
    case EffectType::JumpBoost:      return ImColor(100,220,120);
    case EffectType::Nausea:         return ImColor(80,140,60);
    case EffectType::Regeneration:   return ImColor(240,150,200);
    case EffectType::Resistance:     return ImColor(160,90,50);
    case EffectType::FireResistance: return ImColor(255,150,30);
    case EffectType::WaterBreathing: return ImColor(60,180,220);
    case EffectType::Invisibility:   return ImColor(200,200,220);
    case EffectType::Blindness:      return ImColor(50,50,50);
    case EffectType::NightVision:    return ImColor(90,60,200);
    case EffectType::Hunger:         return ImColor(100,70,30);
    case EffectType::Weakness:       return ImColor(100,80,140);
    case EffectType::Poison:         return ImColor(90,160,50);
    case EffectType::Wither:         return ImColor(60,50,60);
    case EffectType::HealthBoost:    return ImColor(240,80,100);
    case EffectType::Absorption:     return ImColor(255,200,60);
    case EffectType::Saturation:     return ImColor(240,180,180);
    case EffectType::Levitation:     return ImColor(200,200,255);
    case EffectType::FatalPoison:    return ImColor(60,120,20);
    case EffectType::ConduitPower:   return ImColor(40,200,200);
    case EffectType::SlowFalling:    return ImColor(200,230,255);
    case EffectType::BadOmen:        return ImColor(120,20,20);
    case EffectType::VillageHero:    return ImColor(100,255,140);
    case EffectType::Darkness:       return ImColor(30,20,50);
    default:                         return ImColor(200,200,200);
    }
}

const char* PotionHUD::getEffectTextureName(EffectType t)
{
    switch(t) {
    case EffectType::Speed:          return "speed_effect";
    case EffectType::Slowness:       return "slowness_effect";
    case EffectType::Haste:          return "haste_effect";
    case EffectType::MiningFatigue:  return "mining_fatigue_effect";
    case EffectType::Strength:       return "strength_effect";
    case EffectType::JumpBoost:      return "jump_boost_effect";
    case EffectType::Nausea:         return "nausea_effect";
    case EffectType::Regeneration:   return "regeneration_effect";
    case EffectType::Resistance:     return "resistance_effect";
    case EffectType::FireResistance: return "fire_resistance_effect";
    case EffectType::WaterBreathing: return "water_breathing_effect";
    case EffectType::Invisibility:   return "invisibility_effect";
    case EffectType::Blindness:      return "blindness_effect";
    case EffectType::NightVision:    return "night_vision_effect";
    case EffectType::Hunger:         return "hunger_effect";
    case EffectType::Weakness:       return "weakness_effect";
    case EffectType::Poison:         return "poison_effect";
    case EffectType::Wither:         return "wither_effect";
    case EffectType::Levitation:     return "levitation_effect";
    case EffectType::BadOmen:        return "bad_omen_effect";
    case EffectType::VillageHero:    return "village_hero_effect";
    default:                         return nullptr;
    }
}

std::string PotionHUD::romanLevel(int amp)
{
    static const char* roman[] = {"I","II","III","IV","V","VI","VII","VIII","IX","X"};
    if (amp < 0) return "";
    if (amp < 10) return roman[amp];
    return std::to_string(amp+1);
}

std::string PotionHUD::formatTime(int ticks)
{
    if (ticks <= 0)  return "0:00";
    if (ticks > 20 * 3600) return "*:*";
    int total = ticks / 20;
    int m = total / 60;
    int s = total % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

void PotionHUD::onEnable()
{
    mEffects.clear();
    gFeatureManager->mDispatcher->listen<PacketInEvent, &PotionHUD::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &PotionHUD::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,   &PotionHUD::onRenderEvent>(this);

    if (!gPotionElem) {
        gPotionElem = new PotionHudElement();
        gPotionElem->mSize = {220.f, 30.f};
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gPotionElem);
    }
    if (gPotionElem) gPotionElem->mVisible = true;

    if (!ItemTextures::isReady())
        ItemTextures::init();
}

void PotionHUD::onDisable()
{
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &PotionHUD::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &PotionHUD::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,   &PotionHUD::onRenderEvent>(this);
    if (gPotionElem) gPotionElem->mVisible = false;
}

void PotionHUD::onPacketInEvent(PacketInEvent& event)
{
    if (event.mPacket->getId() != PacketID::MobEffect) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    auto pkt = event.getPacket<MobEffectPacket>();
    if (pkt->mRuntimeId != player->getRuntimeID()) return;

    if (pkt->mEventId == MobEffectPacket::Event::Add ||
        pkt->mEventId == MobEffectPacket::Event::Update)
    {
        ActiveEffect ae;
        ae.type          = pkt->mEffectId;
        ae.amplifier     = pkt->mEffectAmplifier;
        ae.durationTicks = pkt->mEffectDurationTicks;
        ae.displayTime   = (float)ae.durationTicks / 20.f;
        ae.introAnim     = 0.f;
        ae.negative      = isNegativeEffect(ae.type);
        mEffects[ae.type] = ae;
    }
    else if (pkt->mEventId == MobEffectPacket::Event::Remove)
    {
        mEffects.erase(pkt->mEffectId);
    }
}

void PotionHUD::onBaseTickEvent(BaseTickEvent& event)
{
    for (auto& [type, ae] : mEffects) {
        if (ae.durationTicks > 0) ae.durationTicks--;
    }
    for (auto it = mEffects.begin(); it != mEffects.end(); ) {
        if (it->second.durationTicks <= 0) it = mEffects.erase(it);
        else ++it;
    }
}

void PotionHUD::onRenderEvent(RenderEvent& event)
{
    if (mEffects.empty()) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    std::vector<ActiveEffect*> list;
    for (auto& [t, ae] : mEffects) {
        if (mHideNegative.mValue && ae.negative) continue;
        list.push_back(&ae);
    }
    if (list.empty()) return;

    if (mSortByTime.mValue) {
        std::sort(list.begin(), list.end(), [](ActiveEffect* a, ActiveEffect* b){
            return a->durationTicks < b->durationTicks;
        });
    }

    float scale      = mScale.mValue;
    float fs         = mFontSize.mValue * scale;
    float rowH       = (fs + 14.f) * scale;
    float width      = 210.f * scale;
    float iconSize   = 18.f * scale;
    float padX       = 10.f * scale;
    float rounding   = mRounding.mValue * scale;

    float totalH = (float)list.size() * rowH + 8.f * scale;

    ImVec2 base = gPotionElem ? gPotionElem->getPos() : ImVec2(20.f, 80.f);
    float x = base.x, y = base.y;

    auto dl = ImGui::GetBackgroundDrawList();
    FontHelper::pushPrefFont(true, false, false);
    float now = ImGui::GetTime();

    // ClickGui-style background
    dl->AddRectFilled({x, y}, {x+width, y+totalH},
        ImColor(24, 24, 28, (int)(235 * mOpacity.mValue)), rounding);
    dl->AddRect({x, y}, {x+width, y+totalH},
        ImColor(255,255,255,18), rounding, 0, 1.f);

    float ry = y + 4.f * scale;
    int rowIdx = 0;

    for (ActiveEffect* ae : list) {
        ae->introAnim = MathUtils::animate(1.f, ae->introAnim, ImGui::GetIO().DeltaTime * 8.f);
        float alpha = ae->introAnim;

        // ── Icon (texture or colored square fallback) ────────────────────────
        float cx = x + padX;
        const char* texName = getEffectTextureName(ae->type);
        auto* tex = texName ? ItemTextures::getTexture(texName) : nullptr;

        if (tex && tex->srv) {
            float iconY = ry + (rowH - iconSize) * 0.5f;
            dl->AddImage((ImTextureID)tex->srv,
                {cx, iconY}, {cx + iconSize, iconY + iconSize},
                {0,0}, {1,1}, ImColor(255,255,255,(int)(255*alpha)));
        } else {
            // Fallback colored square
            ImColor fc = effectColor(ae->type); fc.Value.w = alpha;
            dl->AddRectFilled({cx + 2.f*scale, ry + (rowH - iconSize)*0.5f + 2.f*scale},
                              {cx + iconSize - 2.f*scale, ry + (rowH - iconSize)*0.5f + iconSize - 2.f*scale},
                              fc, 3.f);
        }

        cx += iconSize + 8.f * scale;

        // ── Name ─────────────────────────────────────────────────────────────
        std::string name(magic_enum::enum_name<EffectType>(ae->type));
        std::string displayName;
        for (size_t ci = 0; ci < name.size(); ci++) {
            if (ci > 0 && std::isupper(name[ci])) displayName += ' ';
            displayName += name[ci];
        }
        if (mShowLevel.mValue && ae->amplifier > 0) {
            displayName += " " + romanLevel(ae->amplifier);
        }

        bool isLow = (ae->durationTicks < 100 && ae->durationTicks > 0);
        float blinkAlpha = 1.f;
        if (mBlinkLow.mValue && isLow)
            blinkAlpha = 0.5f + 0.5f * sinf(now * 8.f);

        ImColor textCol = ImColor(230,230,240,(int)(235 * alpha * blinkAlpha));
        dl->AddText(ImGui::GetFont(), fs, {cx, ry + (rowH - fs)*0.5f}, textCol, displayName.c_str());

        // ── Timer (right aligned) ────────────────────────────────────────────
        if (mShowTimer.mValue) {
            std::string timeStr = formatTime(ae->durationTicks);
            float tfs = fs * 0.82f;
            ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(tfs, FLT_MAX, 0, timeStr.c_str());
            float tx = x + width - ts.x - padX;
            ImColor timerCol = isLow
                ? ImColor(1.f, 0.35f + 0.35f*sinf(now*8.f), 0.25f, alpha)
                : ImColor(0.65f, 0.8f, 1.f, alpha * 0.85f);
            dl->AddText(ImGui::GetFont(), tfs, {tx, ry + (rowH - tfs)*0.5f}, timerCol, timeStr.c_str());
        }

        // Separator
        if (rowIdx < (int)list.size() - 1) {
            dl->AddLine({x + padX + iconSize + 4.f*scale, ry + rowH},
                        {x + width - padX, ry + rowH},
                        ImColor(255,255,255,(int)(12*alpha)), 1.f);
        }

        ry += rowH;
        rowIdx++;
    }

    if (gPotionElem) gPotionElem->mSize = {width, totalH};
    ImGui::PopFont();
}