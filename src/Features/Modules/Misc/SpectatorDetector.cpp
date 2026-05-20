#include "SpectatorDetector.hpp"

#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/Network/Packets/SetPlayerGameTypePacket.hpp>
#include <SDK/Minecraft/Network/Packets/UpdatePlayerGameTypePacket.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

// ═══════════════════════════════════════════════════════════════════════════
// ENABLE / DISABLE
// ═══════════════════════════════════════════════════════════════════════════

void SpectatorDetector::onEnable()
{
    mSpectators.clear();
    gFeatureManager->mDispatcher->listen<PacketInEvent,  &SpectatorDetector::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent,  &SpectatorDetector::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,    &SpectatorDetector::onRenderEvent>(this);
}

void SpectatorDetector::onDisable()
{
    gFeatureManager->mDispatcher->deafen<PacketInEvent,  &SpectatorDetector::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,  &SpectatorDetector::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,    &SpectatorDetector::onRenderEvent>(this);
    mSpectators.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPERS
// ═══════════════════════════════════════════════════════════════════════════

std::string SpectatorDetector::getNameById(int64_t id)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return "Unknown";
    auto level = player->getLevel();
    if (!level) return "Unknown";
    auto list = level->getPlayerList();
    if (!list) return "Unknown";

    for (auto& [uuid, entry] : *list)
        if (static_cast<int64_t>(entry.mId) == id)
            return entry.mName;

    return "ID:" + std::to_string(id);
}

// ═══════════════════════════════════════════════════════════════════════════
// PACKET IN — перехватываем смену геймтайпа
// ═══════════════════════════════════════════════════════════════════════════

void SpectatorDetector::onPacketInEvent(PacketInEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    // ── UpdatePlayerGameType (другой игрок меняет режим) ─────────────────
    if (event.mPacket->getId() == PacketID::UpdatePlayerGameType)
    {
        auto pkt = event.getPacket<UpdatePlayerGameTypePacket>();
        int64_t   targetId = pkt->mTargetPlayerUniqueId;
        GameType  gameType = pkt->mPlayerGameType;

        std::string name = getNameById(targetId);

        if (gameType == GameType::Spectator)
        {
            // ── Запоминаем спектатора ────────────────────────────────────
            bool isNew = mSpectators.find(targetId) == mSpectators.end();
            mSpectators[targetId] = name;

            if (isNew && mNotify.mValue) {
                NotifyUtils::notify("Spectator: " + name, 5.f, Notification::Type::Warning);
                ChatUtils::displayClientMessage("§c[!] §eSpectator detected: §f" + name);
            }

            // ── Подменяем Spectator → Creative чтобы видеть игрока ──────
            if (mSpoofCreative.mValue) {
                pkt->mPlayerGameType = GameType::Creative;
                // Пакет будет обработан клиентом уже с Creative,
                // поэтому игрок в спектаторе станет видимым
            }
        }
        else
        {
            // Игрок вышел из спектатора — убираем из списка
            auto it = mSpectators.find(targetId);
            if (it != mSpectators.end()) {
                mSpectators.erase(it);
                if (mNotify.mValue)
                    ChatUtils::displayClientMessage("§a[+] §7Left spectator: §f" + name);
            }
        }

        // Логирование (опционально)
        if (mLogPackets.mValue) {
            const char* gtName = "Unknown";
            switch (gameType) {
                case GameType::Survival:  gtName = "Survival";  break;
                case GameType::Creative:  gtName = "Creative";  break;
                case GameType::Adventure: gtName = "Adventure"; break;
                case GameType::Spectator: gtName = "Spectator"; break;
                default: break;
            }
            ChatUtils::displayClientMessage(
                "§7[SpectDet] §f" + name + " §7→ §e" + gtName);
        }
    }

    // ── SetPlayerGameType (твой собственный режим) ────────────────────────
    if (event.mPacket->getId() == PacketID::SetPlayerGameType && mLogPackets.mValue)
    {
        auto pkt = event.getPacket<SetPlayerGameTypePacket>();
        const char* gtName = "Unknown";
        switch (pkt->mPlayerGameType) {
            case GameType::Survival:  gtName = "Survival";  break;
            case GameType::Creative:  gtName = "Creative";  break;
            case GameType::Adventure: gtName = "Adventure"; break;
            case GameType::Spectator: gtName = "Spectator"; break;
            default: break;
        }
        ChatUtils::displayClientMessage("§7[SpectDet] §fYour mode → §e" + std::string(gtName));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// TICK — периодически чистим игроков которые вышли с сервера
// ═══════════════════════════════════════════════════════════════════════════

void SpectatorDetector::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    auto level = player->getLevel();
    if (!level) return;

    auto list = level->getPlayerList();
    if (!list || list->empty()) return;

    // Удаляем записи о спектаторах которых уже нет в PlayerList
    std::vector<int64_t> toRemove;
    for (auto& [id, name] : mSpectators) {
        bool found = false;
        for (auto& [uuid, entry] : *list)
            if (static_cast<int64_t>(entry.mId) == id) { found = true; break; }
        if (!found) toRemove.push_back(id);
    }
    for (auto id : toRemove) {
        mSpectators.erase(id);
        if (mNotify.mValue)
            ChatUtils::displayClientMessage("§8[SpectDet] Player left server.");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// RENDER — красивый список спектаторов
// ═══════════════════════════════════════════════════════════════════════════

void SpectatorDetector::onRenderEvent(RenderEvent& event)
{
    if (!mShowList.mValue) return;
    if (mSpectators.empty()) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    FontHelper::pushPrefFont(false);

    const float x        = 10.f;
    const float y        = 100.f;
    const float padX     = 10.f;
    const float padY     = 7.f;
    const float lineH    = 19.f;
    const float headerH  = 28.f;
    const float width    = 210.f;
    const float rounding = 8.f;

    int count = (int)mSpectators.size();
    float totalH = headerH + count * lineH + padY;

    // ── Blur glassmorphism ────────────────────────────────────────────────
    ImRenderUtils::addBlur(ImVec4(x, y, x + width, y + totalH), 3.5f, rounding);

    // ── Dark background ───────────────────────────────────────────────────
    drawList->AddRectFilled(
        {x, y}, {x + width, y + totalH},
        ImColor(12, 12, 16, 210), rounding);

    // ── Accent header strip ───────────────────────────────────────────────
    ImColor accentA = ColorUtils::getThemedColor(0.f);
    ImColor accentB = ColorUtils::getThemedColor(90.f);
    accentA.Value.w = 0.92f;
    accentB.Value.w = 0.92f;

    drawList->AddRectFilledMultiColor(
        {x, y}, {x + width, y + headerH},
        accentA, accentB, accentB, accentA);

    // Round top corners of header
    drawList->AddRectFilled(
        {x, y + headerH - rounding}, {x + width, y + headerH},
        accentB); // fill the bottom square part of header

    // ── Shadow under card ─────────────────────────────────────────────────
    drawList->AddShadowRect(
        {x, y}, {x + width, y + totalH},
        ImColor(0, 0, 0, 70), 18.f, {0, 4}, 0, rounding);

    // ── Header text ───────────────────────────────────────────────────────
    std::string header = "Spectators  (" + std::to_string(count) + ")";
    ImVec2 hts = ImGui::GetFont()->CalcTextSizeA(13.f, FLT_MAX, 0, header.c_str());
    drawList->AddText(ImGui::GetFont(), 13.f,
        {x + (width - hts.x) * 0.5f, y + (headerH - hts.y) * 0.5f},
        ImColor(255, 255, 255, 255), header.c_str());

    // ── Top glass rim ─────────────────────────────────────────────────────
    drawList->AddRectFilled(
        {x + rounding * 0.5f, y},
        {x + width - rounding * 0.5f, y + 1.f},
        ImColor(255, 255, 255, 22), 0.5f);

    // ── Spectator entries ─────────────────────────────────────────────────
    float ty = y + headerH + 4.f;
    int idx  = 0;
    for (auto& [id, name] : mSpectators)
    {
        // Alternating row tint
        if (idx % 2 == 0) {
            drawList->AddRectFilled(
                {x + 3.f, ty - 1.f},
                {x + width - 3.f, ty + lineH - 1.f},
                ImColor(255, 255, 255, 8), 4.f);
        }

        // Coloured dot (cycles through theme colors)
        ImColor dot = ColorUtils::getThemedColor((float)idx * 35.f);
        dot.Value.w = 0.9f;
        drawList->AddCircleFilled({x + padX + 4.f, ty + lineH * 0.5f - 1.f}, 3.5f, dot, 8);

        // Name
        drawList->AddText(ImGui::GetFont(), 12.5f,
            {x + padX + 12.f, ty + (lineH - 12.5f) * 0.5f},
            ImColor(230, 230, 240, 220), name.c_str());

        // "SPEC" badge
        if (!mSpoofCreative.mValue) {
            // Only show badge if we're NOT already spoofing them as creative
            drawList->AddRectFilled(
                {x + width - 36.f, ty + 3.f},
                {x + width - 6.f, ty + lineH - 3.f},
                ImColor(dot.Value.x, dot.Value.y, dot.Value.z, 0.3f), 3.f);
            ImVec2 bs = ImGui::GetFont()->CalcTextSizeA(10.f, FLT_MAX, 0, "SPEC");
            drawList->AddText(ImGui::GetFont(), 10.f,
                {x + width - 36.f + (30.f - bs.x) * 0.5f, ty + (lineH - bs.y) * 0.5f},
                ImColor(dot.Value.x, dot.Value.y, dot.Value.z, 0.9f), "SPEC");
        } else {
            drawList->AddRectFilled(
                {x + width - 40.f, ty + 3.f},
                {x + width - 6.f, ty + lineH - 3.f},
                ImColor(0.2f, 0.8f, 0.4f, 0.25f), 3.f);
            ImVec2 bs = ImGui::GetFont()->CalcTextSizeA(10.f, FLT_MAX, 0, "CREAT");
            drawList->AddText(ImGui::GetFont(), 10.f,
                {x + width - 40.f + (34.f - bs.x) * 0.5f, ty + (lineH - bs.y) * 0.5f},
                ImColor(0.3f, 1.f, 0.5f, 0.95f), "CREAT");
        }

        ty += lineH;
        idx++;
    }

    ImGui::PopFont();
}