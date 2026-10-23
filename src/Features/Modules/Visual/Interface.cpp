//
// Created by vastrakai on 7/1/2024.
//

#include <Features/Events/ActorRenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/DrawImageEvent.hpp>
#include <Features/Events/ModuleStateChangeEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/PacketOutEvent.hpp>
#include <Features/Events/PreGameCheckEvent.hpp>
#include <Features/Events/RenderEvent.hpp>

#include <Features/Modules/Visual/ClickGui.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <Hook/Hooks/RenderHooks/ActorRenderDispatcherHook.hpp>
#include <Hook/Hooks/RenderHooks/HoverTextRendererHook.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/mce.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Network/Packets/Packet.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerAuthInputPacket.hpp>
#include <Utils/FontHelper.hpp>

#include <SDK/Minecraft/Actor/SyncedPlayerMovementSettings.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/World/Level.hpp>

#include <algorithm>
#include <cmath>

#ifdef __DEBUG__
std::vector<unsigned char> gFsBytes2 = { 0x0f, 0x85 };
DEFINE_PATCH_FUNC(patchFullStack, SigManager::ResourcePackManager_composeFullStackBp, gFsBytes2);
#endif

// please someone make this in a class or struct cuz it gives me aids
// Define a mapping from Minecraft color codes to RGBA colors
std::unordered_map<char, ImColor> mColorMap = {
    {'0', ImColor(0, 0, 0)},        // Black
    {'1', ImColor(0, 0, 170)},      // Dark Blue
    {'2', ImColor(0, 170, 0)},      // Dark Green
    {'3', ImColor(0, 170, 170)},    // Dark Aqua
    {'4', ImColor(170, 0, 0)},      // Dark Red
    {'5', ImColor(170, 0, 170)},    // Dark Purple
    {'6', ImColor(255, 170, 0)},    // Gold
    {'7', ImColor(170, 170, 170)},  // Gray
    {'8', ImColor(85, 85, 85)},     // Dark Gray
    {'9', ImColor(85, 85, 255)},    // Blue
    {'a', ImColor(85, 255, 85)},    // Green
    {'b', ImColor(85, 255, 255)},   // Aqua
    {'c', ImColor(255, 85, 85)},    // Red
    {'d', ImColor(255, 85, 255)},   // Light Purple
    {'e', ImColor(255, 255, 85)},   // Yellow
    {'f', ImColor(255, 255, 255)},  // White
    {'r', ImColor(255, 255, 255)}   // Reset
};

template <typename T>
std::string combine(T t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

template <typename T, typename... Args>
std::string combine(T t, Args... args)
{
    std::stringstream ss;
    ss << t << combine(args...);
    return ss.str();
}

float pYaw;
float pOldYaw;

float pHeadYaw;
float pOldHeadYaw;

float pPitch;
float pOldPitch;

float pBodyYaw;
float pOldBodyYaw;

float pLerpedYaw;
float pLerpedHeadYaw;
float pLerpedPitch;
float pLerpedBodyYaw;

bool usingPaip = false;

namespace
{
    // Сглаживание здесь решает ровно одну задачу: спрятать «ступеньки»
    // пакетов 20 Гц (голова идёт рывками по 1–2°). Резкий поворот сглаживать
    // нельзя — голова «догоняла» камеру несколько кадров, и это читалось как
    // задержка. Правило простое: обычное изменение — плавно, скачок — сразу.
    constexpr float kInterfaceLerpSpeed = 26.f; // 1/с, не зависит от FPS
    constexpr float kInterfaceSnapDeg   = 45.f; // скачок больше — показываем как есть
    constexpr float kInterfaceMaxLagDeg = 20.f; // насколько модель может отставать от правды

    float interfaceWrap(float deg)
    {
        return MathUtils::wrap(deg, -180.f, 180.f);
    }

    float interfaceFollow(float current, float target, float k, float snapDeg)
    {
        const float diff = interfaceWrap(target - current);
        if (!std::isfinite(diff)) return interfaceWrap(target);
        if (std::fabs(diff) >= snapDeg) return interfaceWrap(target);
        return interfaceWrap(current + diff * k);
    }

    // Шаг сглаживания. Вызывается ДО того, как модель рисуется (см.
    // ActorRenderEvent) — RenderEvent приходит уже после отрисовки мира.
    void interfaceStepSmoothed()
    {
        const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.0001f, 0.1f);
        const float k  = 1.f - std::exp(-kInterfaceLerpSpeed * dt);

        pLerpedYaw     = interfaceFollow(pLerpedYaw,     pYaw,     k, kInterfaceSnapDeg);
        pLerpedHeadYaw = interfaceFollow(pLerpedHeadYaw, pHeadYaw, k, kInterfaceSnapDeg);
        pLerpedPitch   = interfaceFollow(pLerpedPitch,   pPitch,   k, kInterfaceSnapDeg);
        pLerpedBodyYaw = interfaceFollow(pLerpedBodyYaw, pBodyYaw, k, kInterfaceSnapDeg);
    }

    // Сглаженное значение, но не дальше maxLag от настоящего.
    float interfacePickLive(float smoothed, float live, float maxLag)
    {
        const float diff = interfaceWrap(live - smoothed);
        return (std::isfinite(diff) && std::fabs(diff) > maxLag) ? live : smoothed;
    }
}

// Returns the FontHelper key string for the currently selected FontType.
// Mntsb is the only text font the client ships with, so this is a fixed key.
static std::string fontTypeToKey(Interface::FontType)
{
    return "mntsb";
}

void Interface::onEnable()
{
    FontHelper::setCurrentFont(fontTypeToKey(mFont.mValue));
}

void Interface::onDisable()
{
#ifdef __DEBUG__
    patchFullStack(false);
#endif
}

void Interface::renderHoverText()
{
    static EasingUtil inEase;

    (HoverTextRender::mTimeDisplayed != 0 && gFeatureManager->mModuleManager->getModule<ClickGui>()->mEnabled != true) ?
            inEase.incrementPercentage(ImRenderUtils::getDeltaTime() * 2)
            : inEase.decrementPercentage(ImRenderUtils::getDeltaTime() * 4);

    float inScale = HoverTextRender::mTimeDisplayed != 0 && gFeatureManager->mModuleManager->getModule<ClickGui>()->mEnabled != true ? inEase.easeOutExpo() : inEase.easeOutBack();

    if (inEase.isPercentageMax())
        inScale = 1;

    if (inScale < 0.01)
        return;

    glm::vec2 mPos = HoverTextRender::mInfo.mPos;
    glm::vec2 mTextPos = glm::vec2(mPos.x + 6, mPos.y + 6); // It looks better this way than getting it from HoverTextRenderer class

    float mTextSize = 1.25 * inScale;

    std::string mMessage = HoverTextRender::mInfo.mText;
    std::string mNoneColoredText = ColorUtils::removeColorCodes(HoverTextRender::mInfo.mText);

    ImColor mCurrentColor = ImColor(255, 255, 255);

    float mMeasurementX = ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, -1, mNoneColoredText.c_str()).x;
    float mMeasurementY = ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, -1, mNoneColoredText.c_str()).y;

    ImVec4 mRect = ImVec4(mPos.x, mPos.y, mPos.x + mMeasurementX + 12, mPos.y + mMeasurementY + 12);

    ImRenderUtils::addBlur(mRect, 3 * inScale, 10);

    ImRenderUtils::fillRectangle(mRect, ImColor(0, 0, 0), 0.78f * inScale, 10);

    for (size_t j = 0; j < mMessage.length(); ) {
        char c = mMessage[j];

        if (c == '§' && j + 1 < mMessage.length()) {
            char colorCode = mMessage[j + 1];
            if (mColorMap.find(colorCode) != mColorMap.end()) {
                mCurrentColor = mColorMap[colorCode];
                j += 2;
            } else {
                j++;
            }
            continue;
        }

        if (c == '\n') {
            mTextPos.x = mPos.x + 6;
            mTextPos.y += ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, 0, "\n").y;
            j++;
            continue;
        }

        size_t len = 1;
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 0xc0 && uc < 0xe0) len = 2;
        else if (uc >= 0xe0 && uc < 0xf0) len = 3;
        else if (uc >= 0xf0 && uc < 0xf8) len = 4;

        if (j + len > mMessage.length()) len = mMessage.length() - j;
        std::string mString = mMessage.substr(j, len);
        j += len;

        if (len == 1 && (uc < 32 || uc == 127)) {
            continue;
        }

        ImRenderUtils::drawText(mTextPos, mString, mCurrentColor, mTextSize, inScale, false);

        mTextPos.x += ImGui::GetFont()->CalcTextSizeA(mTextSize * 18, FLT_MAX, -1, mString.c_str()).x;
    }

    HoverTextRender::mTimeDisplayed = 0;
}


void Interface::onModuleStateChange(ModuleStateChangeEvent& event)
{
    if (event.mModule == this)
    {
        event.setCancelled(true);
    }
}

void Interface::onPregameCheckEvent(PreGameCheckEvent& event)
{
#ifdef __DEBUG__
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player || ! mForcePackSwitching.mValue) return;

    std::string screenName = ClientInstance::get()->getScreenName();

    // prevent other screens from breaking
    if (screenName.contains("screen_world_controls_and_settings") && !screenName.contains("global_texture_pack_tab")) return;

    event.setPreGame(true);
#endif
}

void Interface::onRenderEvent(RenderEvent& event)
{
    // Apply font changes immediately; Interface is always enabled, so relying
    // on onEnable alone made the selector appear broken until a restart.
    static int lastFont = -1;
    if (lastFont != mFont.as<int>()) {
        lastFont = mFont.as<int>();
        FontHelper::setCurrentFont(fontTypeToKey(mFont.mValue));
    }

    auto player = ClientInstance::get()->getLocalPlayer();
    static bool lastPlayerState = false;

    //renderHoverText();

#ifdef __DEBUG__
    if (player && mForcePackSwitching.mValue)
    {
        patchFullStack(true);
    } else
    {
        patchFullStack(false);
    }
#endif

    if (player && !lastPlayerState)
    {
        usingPaip = false;
    }

    // Один шаг сглаживания за кадр (на случай, когда своя модель не рисуется,
    // например первый человек: ActorRenderEvent тогда не приходит).
    interfaceStepSmoothed();
}

void Interface::onActorRenderEvent(ActorRenderEvent& event)
{
    if (event.isCancelled()) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    
    if (event.mEntity != player) return;
    if (*event.mPos == glm::vec3(0.f, 0.f, 0.f) && *event.mRot == glm::vec2(0.f, 0.f)) return;

    bool firstPerson = ClientInstance::get()->getOptions()->mThirdPerson->value == 0;
    if (firstPerson && !player->getFlag<RenderCameraComponent>()) return;


    const auto actorRotations = event.mEntity->getActorRotationComponent();
    const auto headRotations = event.mEntity->getActorHeadRotationComponent();
    const auto bodyRotations = event.mEntity->getMobBodyRotationComponent();
    if (!actorRotations || !headRotations || !bodyRotations) return;

    float realOldPitch = actorRotations->mOldPitch;
    float realPitch = actorRotations->mPitch;
    float realHeadRot = headRotations->mHeadRot;
    float realOldHeadRot = headRotations->mOldHeadRot;
    float realBodyYaw = bodyRotations->yBodyRot;
    float realOldBodyYaw = bodyRotations->yOldBodyRot;

    // Сглаживаем ИМЕННО ПЕРЕД отрисовкой: RenderEvent приходит уже после того,
    // как мир нарисован, поэтому раньше модель всегда использовала значения
    // прошлого кадра (+1 кадр задержки к тем, что давал сам фильтр).
    interfaceStepSmoothed();

    // И не даём фильтру отставать от настоящей ротации больше чем на 20°:
    // резкий поворот (Aura, Scaffold, мышь) показывается сразу.
    const float shownPitch = interfacePickLive(pLerpedPitch,   realPitch,   kInterfaceMaxLagDeg);
    const float shownHead  = interfacePickLive(pLerpedHeadYaw, realHeadRot, kInterfaceMaxLagDeg);
    const float shownBody  = interfacePickLive(pLerpedBodyYaw, realBodyYaw, kInterfaceMaxLagDeg);

    actorRotations->mOldPitch = shownPitch;
    actorRotations->mPitch = shownPitch;
    headRotations->mHeadRot = shownHead;
    headRotations->mOldHeadRot = shownHead;
    bodyRotations->yBodyRot = shownBody;
    bodyRotations->yOldBodyRot = shownBody;

    auto original = event.mDetour->getOriginal<&ActorRenderDispatcherHook::render>();
    original(event._this, event.mEntityRenderContext, event.mEntity, event.mCameraTargetPos, event.mPos, event.mRot, event.mIgnoreLighting);
    event.cancel();

    actorRotations->mOldPitch = realOldPitch;
    actorRotations->mPitch = realPitch;
    headRotations->mHeadRot = realHeadRot;
    headRotations->mOldHeadRot = realOldHeadRot;
    bodyRotations->yBodyRot = realBodyYaw;
    bodyRotations->yOldBodyRot = realOldBodyYaw;
}

void Interface::onDrawImageEvent(DrawImageEvent& event)
{
    if (!mSlotEasing.mValue) return;
    // Wait for ImGui to be initialized
    if (!ImGui::GetCurrentContext()) return;

    static glm::vec2 hotbarPos = {};
    auto path = event.mTexture->mTexture->mFilePath.c_str();

    // textures/ui/selected_hotbar_slot
    // If the selected hotbar slot is being drawn
    if (strcmp(path, "textures/ui/selected_hotbar_slot") == 0)
    {
        float deltaTime = ImGui::GetIO().DeltaTime;
        if (hotbarPos.x == 0 || hotbarPos.y == 0) hotbarPos = *event.mPos;
        hotbarPos.x = MathUtils::lerp(hotbarPos.x, event.mPos->x, deltaTime * mSlotEasingSpeed.mValue);
        hotbarPos.y = event.mPos->y;
        *event.mPos = hotbarPos;
    }
}


void Interface::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;   // мир выгружается — BodyYaw::updateRenderAngles читает getPos()

    BodyYaw::updateRenderAngles(player, pYaw);
    pOldBodyYaw = pBodyYaw;
    pBodyYaw = BodyYaw::bodyYaw;

    // Apply font setting on change
    static FontType lastFont = (FontType)-1;
    if (mFont.mValue != lastFont) {
        lastFont = mFont.mValue;
        FontHelper::setCurrentFont(fontTypeToKey(mFont.mValue));
    }
}

void Interface::onPacketOutEvent(PacketOutEvent& event)
{
    if (!event.mPacket) return;

    const PacketID id = event.mPacket->getId();
    if (id != PacketID::PlayerAuthInput && id != PacketID::MovePlayer) return;

    // Ротация приходит в том пакете, которым реально двигается игрок:
    // PlayerAuthInput на server-authoritative серверах, MovePlayer на
    // клиент-авторитетных. Раньше тип выбирался по флагу, который выставлялся
    // при виде первого PAIP, — на части серверов флаг был неверным, и модель
    // вообще не видела поворотов (голова замирала). Читаем оба пакета.
    glm::vec2 rot{};
    float headYaw = 0.f;

    if (id == PacketID::PlayerAuthInput)
    {
        auto paip = event.getPacket<PlayerAuthInputPacket>();
        if (!paip) return;
        usingPaip = true;
        rot = paip->mRot;
        headYaw = paip->mYHeadRot;
    }
    else
    {
        auto mpp = event.getPacket<MovePlayerPacket>();
        if (!mpp) return;
        // Телепорт/респавн — служебная ротация, это не поворот игрока.
        if (mpp->mResetPosition == PositionMode::Teleport ||
            mpp->mResetPosition == PositionMode::Respawn)
            return;
        rot = mpp->mRot;
        headYaw = mpp->mYHeadRot;
    }

    if (!std::isfinite(rot.x) || !std::isfinite(rot.y) || !std::isfinite(headYaw))
        return;

    pOldYaw = pYaw;
    pOldPitch = pPitch;
    pOldBodyYaw = pBodyYaw;
    pYaw = rot.y;
    pPitch = rot.x;
    pHeadYaw = headYaw;
}
