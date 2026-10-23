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
#include <chrono>
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

// Первый пакет ещё не пришёл — показывать модели нули нельзя.
bool gInterfaceRotReady = false;
// Когда пришёл последний пакет с ротацией. На клиент-авторитетных серверах
// MovePlayer идёт только во время движения, и без этой проверки голова
// замирала бы, стоило остановиться.
std::chrono::steady_clock::time_point gInterfacePktTp{};

namespace
{
    // Как это устроено (и почему раньше дёргалось).
    //
    // Модель рисуется из одного источника — ротации, которая уходит в пакет
    // (её же правят Aura и Scaffold). Пакет идёт 20 Гц, поэтому между тиками
    // голова двигалась «ступеньками» по 5–20° — их и гасит фильтр.
    //
    // Раньше рядом с этим значением брали ещё и «живое» значение компоненты и
    // переключались между ними по порогу 20°, а сам фильтр снимался дважды за
    // кадр. На быстрой камере фильтр постоянно отставал больше чем на 20° →
    // выбор между двумя источниками щёлкал туда-сюда КАЖДЫЙ кадр: это и было
    // дрожание головы и корпуса.
    constexpr float kInterfaceLerpSpeed = 30.f; // 1/с, голова и наклон
    constexpr float kInterfaceBodySpeed = 16.f; // 1/с, корпус — всегда тяжелее

    float interfaceWrap(float deg)
    {
        return MathUtils::wrap(deg, -180.f, 180.f);
    }

    // Экспоненциальное приближение, не зависящее от FPS. Без порогов-скачков:
    // любой скачок всё равно проходится за ~3 кадра, а порог только добавлял
    // рывки в тот момент, когда камера его пересекала.
    float interfaceFollow(float current, float target, float k)
    {
        const float diff = interfaceWrap(target - current);
        if (!std::isfinite(diff)) return interfaceWrap(target);
        return interfaceWrap(current + diff * k);
    }

    // Привести угол к той же «ветке», что и reference (0..360 против -180..180).
    float interfaceMatchBranch(float value, float reference)
    {
        value = interfaceWrap(value);
        while (value - reference > 180.f)  value -= 360.f;
        while (value - reference < -180.f) value += 360.f;
        return value;
    }

    // Сколько миллисекунд назад ушёл последний пакет с ротацией.
    double pPktAgeMs()
    {
        const auto dt = std::chrono::steady_clock::now() - gInterfacePktTp;
        return std::chrono::duration<double, std::milli>(dt).count();
    }

    // Шаг модели: ровно один за кадр, ДО отрисовки игрока. Время меряем сами:
    // ImGui::GetIO().DeltaTime обновляется в NewFrame, а мир может рисоваться
    // и до, и после него — шаг «плыл» бы вместе с порядком кадров.
    void interfaceStepModel(Actor* player, float pitch, float headYaw)
    {
        static auto lastStep = std::chrono::steady_clock::now();
        const auto nowTp = std::chrono::steady_clock::now();
        const float rawDt = std::chrono::duration<float>(nowTp - lastStep).count();
        if (rawDt < 0.001f) return;   // второй вызов в том же кадре
        lastStep = nowTp;

        const float dt = std::clamp(rawDt, 0.001f, 0.1f);
        const float kHead = 1.f - std::exp(-kInterfaceLerpSpeed * dt);
        const float kBody = 1.f - std::exp(-kInterfaceBodySpeed * dt);

        pLerpedYaw     = interfaceFollow(pLerpedYaw,     headYaw, kHead);
        pLerpedHeadYaw = interfaceFollow(pLerpedHeadYaw, headYaw, kHead);
        pLerpedPitch   = interfaceFollow(pLerpedPitch,   pitch,   kHead);

        // Корпус. Раньше он приходил из BodyYaw (20 Гц, из пакетной ротации)
        // с клампом ±75° от головы: из-за клампа тело не могло развернуться,
        // когда игрок идёт спиной вперёд, и вообще «не поворачивалось».
        // Теперь корпус считается покадрово и смотрит туда, КУДА ИДЁМ:
        //   • вперёд/по диагонали — почти как голова;
        //   • вбок (A/D) — тело разворачивается по ходу, как в ваниле;
        //   • назад (S) — тело разворачивается на 180°, голова остаётся на
        //     камере: это и есть «поворот тела при ходьбе назад»;
        //   • стоим — тело плавно догоняет голову.
        float bodyTarget = pLerpedHeadYaw;
        if (player)
        {
            if (auto* sv = player->getStateVectorComponent())
            {
                const glm::vec3& v = sv->mVelocity;
                if (std::sqrt(v.x * v.x + v.z * v.z) > 0.12f)
                    bodyTarget = glm::degrees(std::atan2(v.z, v.x)) - 90.f; // направление хода
            }
        }

        pLerpedBodyYaw = interfaceFollow(pLerpedBodyYaw, bodyTarget, kBody);
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

    if (static_cast<bool>(player) != lastPlayerState)
    {
        lastPlayerState = static_cast<bool>(player);
        usingPaip = false;
        // Мир сменился — фильтр начинает с настоящих углов нового мира.
        gInterfaceRotReady = false;
    }

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

    // Своей модели ещё нет (мир только загрузился) — не трогаем компоненты,
    // иначе на один кадр она встанет в нули.
    if (!gInterfaceRotReady) return;

    // Один шаг за кадр, прямо перед отрисовкой. RenderEvent приходит уже после
    // того, как мир нарисован, поэтому раньше модель всегда показывала значения
    // прошлого кадра — поверх задержки самого фильтра.
    // Источник — РОТАЦИЯ ИЗ ПАКЕТА, а не компонента актора. Разница
    // принципиальная: Aura и Scaffold правят именно пакет, и только так их
    // доводка прицела видна на модели (раньше модель брала компоненту и
    // показать поворот на цель не могла вообще).
    //
    // Если пакетов давно не было (стоим на клиент-авторитетном сервере),
    // берём живые компоненты — иначе голова замирала бы на месте.
    const bool fresh = pPktAgeMs() < 150.0;
    const float srcPitch = fresh ? pPitch : realPitch;
    const float srcHead  = fresh ? pHeadYaw : realHeadRot;

    interfaceStepModel(player, srcPitch, srcHead);

    // old = current: модель должна показывать РОВНО посчитанный угол. Старое
    // значение здесь не нужно — угол уже сглажен покадрово, а «догоняющая»
    // интерполяция игры поверх этого давала лишний шаг на каждом тике.
    const float shownPitch = pLerpedPitch;
    const float shownHead  = interfaceMatchBranch(pLerpedHeadYaw, srcHead);
    const float shownBody  = interfaceMatchBranch(pLerpedBodyYaw, shownHead);

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

    gInterfacePktTp = std::chrono::steady_clock::now();

    if (!gInterfaceRotReady)
    {
        // Первый пакет: фильтр стартует с настоящих углов, иначе модель
        // «доезжала» бы к ним из нуля прямо на глазах.
        gInterfaceRotReady = true;
        pLerpedYaw = pLerpedHeadYaw = interfaceWrap(headYaw);
        pLerpedPitch = pPitch = rot.x;
        pLerpedBodyYaw = pBodyYaw = interfaceWrap(headYaw);
    }

    pOldYaw = pYaw;
    pOldPitch = pPitch;
    pOldBodyYaw = pBodyYaw;
    pYaw = rot.y;
    pPitch = rot.x;
    pHeadYaw = headYaw;
}
