//
// Created by alteik on 04/09/2024.
//

#include "AutoClicker.hpp"
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Visual/ClickGui.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/ActorRotationComponent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/GameMode.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <Features/Modules/Misc/AntiBot.hpp>
#include <Hook/Hooks/MiscHooks/MouseHook.hpp>
#include <SDK/Minecraft/Actor/Components/RuntimeIDComponent.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>

// =========================================================
// KEY BIND HELPERS
// =========================================================
const char* AutoClicker::getKeyName(int vk) {
    static char buf[32];
    switch (vk) {
    case 0: return "None";
    case VK_LBUTTON: return "LMB";
    case VK_RBUTTON: return "RMB";
    case VK_MBUTTON: return "MMB";
    case VK_BACK: return "Back";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt";
    case VK_ESCAPE: return "Esc";
    case VK_SPACE: return "Space";
    case VK_DELETE: return "Del";
    case VK_INSERT: return "Ins";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_CAPITAL: return "CapsLock";
    default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buf, 32, "F%d", vk - VK_F1 + 1); return buf; }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { snprintf(buf, 32, "Num%d", vk - VK_NUMPAD0); return buf; }
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) { snprintf(buf, 32, "%c", (char)vk); return buf; }
    snprintf(buf, 32, "0x%02X", vk);
    return buf;
}

bool AutoClicker::isAnyKeyHeld() {
    for (int vk = 8; vk <= 254; vk++) {
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;
        if (vk == VK_LSHIFT || vk == VK_RSHIFT) continue;
        if (vk == VK_LCONTROL || vk == VK_RCONTROL) continue;
        if (vk == VK_LMENU || vk == VK_RMENU) continue;
        if (vk == VK_LWIN || vk == VK_RWIN) continue;
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
        if (vk == VK_ESCAPE) continue;
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    return false;
}

int AutoClicker::findHeldKey() {
    for (int vk = 8; vk <= 254; vk++) {
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;
        if (vk == VK_LSHIFT || vk == VK_RSHIFT) continue;
        if (vk == VK_LCONTROL || vk == VK_RCONTROL) continue;
        if (vk == VK_LMENU || vk == VK_RMENU) continue;
        if (vk == VK_LWIN || vk == VK_RWIN) continue;
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
        if (vk == VK_ESCAPE) continue;
        if (GetAsyncKeyState(vk) & 0x8000) return vk;
    }
    return -1;
}

bool AutoClicker::isKeyJustPressed(int vk) {
    if (vk <= 0) return false;
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool was = mKeyStates[vk];
    mKeyStates[vk] = down;
    return down && !was;
}

// =========================================================
// ENABLE / DISABLE
// =========================================================
void AutoClicker::onEnable() {
    gFeatureManager->mDispatcher->listen<RenderEvent, &AutoClicker::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &AutoClicker::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<LookInputEvent, &AutoClicker::onLookInputEvent>(this);
    randomizeCPS();
    mIsSpinning = false;
    m360CurrentAngle = 0.f;
    m360Progress = 0.f;
    m360Duration = 0.f;
}

void AutoClicker::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &AutoClicker::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &AutoClicker::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<LookInputEvent, &AutoClicker::onLookInputEvent>(this);
}

std::vector<int> AutoClicker::getToolAbuseSlots(Actor* player)
{
    auto supplies = player->getSupplies();
    auto container = supplies->getContainer();
    
    std::map<int, std::vector<int>, std::greater<int>> groupedSlots; 
    
    for (int i = 0; i < 9; i++) {
        auto stack = container->getItem(i);
        if (!stack || !stack->mItem) continue;
        int score = ItemUtils::getItemValue(stack);
        groupedSlots[score].push_back(i);
    }

    std::vector<int> pool;
    for (auto& [score, slots] : groupedSlots) {
        if (pool.empty() && slots.size() >= 2) {
            for (size_t i = 0; i < std::min<size_t>(slots.size(), 3); i++) {
                pool.push_back(slots[i]);
            }
            break;
        }
        
        for (int slot : slots) {
            if (pool.size() < 3) {
                pool.push_back(slot);
            }
        }
        
        if (pool.size() >= 3) break;
    }
    
    return pool;
}

void AutoClicker::tryToolAbuseSwap(Actor* player, HitResult* hitres)
{
    if (!mToolAbuse.mValue) return;

    static uint64_t lastSwapTime = 0;
    if (NOW - lastSwapTime < (uint64_t)mToolAbuseSwapDelay.as<int>()) return;

    if (mToolAbuseOnlyOnHit.mValue) {
        if (!hitres || hitres->mType != ENTITY) return;
    }

    auto supplies = player->getSupplies();
    if (!supplies) return;

    std::vector<int> slotsToSwap = getToolAbuseSlots(player);
    if (slotsToSwap.size() < 2) return;

    int currentSlot = supplies->mSelectedSlot;
    auto it = std::find(slotsToSwap.begin(), slotsToSwap.end(), currentSlot);
    
    if (it != slotsToSwap.end()) {
        int index = std::distance(slotsToSwap.begin(), it);
        int nextIndex = (index + 1) % slotsToSwap.size();
        supplies->mSelectedSlot = slotsToSwap[nextIndex];
    } else {
        supplies->mSelectedSlot = slotsToSwap[0];
    }
    
    lastSwapTime = NOW;
}

void AutoClicker::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    // --------------------------------------------------
    // KEYBIND CAPTURE
    // --------------------------------------------------
    if (mIsBindingKey) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            mIsBindingKey = false;
            mBindWaitRelease = false;
            return;
        }
        if (mBindWaitRelease) {
            if (!isAnyKeyHeld()) mBindWaitRelease = false;
            return;
        }
        int pressed = findHeldKey();
        if (pressed > 0) {
            mTriggerKey = pressed;
            mIsBindingKey = false;
            mBindWaitRelease = false;
            mKeyStates[pressed] = true;
            ChatUtils::displayClientMessage(std::string("§a360 Bind set to: §f") + getKeyName(pressed));
        }
        return;
    }

    // --------------------------------------------------
    // 360 TRIGGER — только по нажатию бинда
    // --------------------------------------------------
    if (m360Spin.mValue && !mIsSpinning) {
        if (isKeyJustPressed(mTriggerKey)) {
            mIsSpinning = true;
            m360CurrentAngle = 0.f;
            m360Progress = 0.f;
            m360Duration = 0.f;
            m360Direction = m360RandomDir.mValue ? (MathUtils::random(0, 1) == 0 ? 1.f : -1.f) : 1.f;
        }
    }
}

// =========================================================
// 360 SPIN — парабола: резкий старт, плавный конец
// =========================================================
void AutoClicker::onLookInputEvent(LookInputEvent& event) {
    if (!m360Spin.mValue || !mIsSpinning) return;
    if (!event.mCameraDirectLookComponent) return;

    if (m360Smooth.mValue) {
        // Первый кадр — запоминаем yaw и вычисляем случайную длительность
        if (m360Duration <= 0.0f) {
            m360StartYaw = event.mCameraDirectLookComponent->mRotRads.x;
            // Базовая длительность от Speed, но каждый спин немного разный (±20%)
            // Speed 45 ~ 30 тиков, Speed 90 ~ 15 тиков, Speed 20 ~ 70 тиков
            float baseDuration = 1350.0f / std::max(m360Speed.mValue, 1.0f);
            float randomFactor = 0.8f + (MathUtils::random(0, 40) / 100.0f);
            m360Duration = baseDuration * randomFactor;
            if (m360Duration < 8.0f) m360Duration = 8.0f;
        }

        m360Progress += 1.0f;
        float t = m360Progress / m360Duration;
        if (t > 1.0f) t = 1.0f;

        // ПАРАБОЛА ease-out: angle = 1 - (1-t)^2
        // t=0.25 -> 44% пути (157°) — уже почти половина
        // t=0.50 -> 75% пути (270°) — резкий старт!
        // t=0.75 -> 94% пути (337°) — остаток тянется медленно
        float ease = 1.0f - (1.0f - t) * (1.0f - t);

        float targetYaw = m360StartYaw + m360Direction * (2.0f * IM_PI) * ease;

        while (targetYaw > IM_PI) targetYaw -= 2.0f * IM_PI;
        while (targetYaw < -IM_PI) targetYaw += 2.0f * IM_PI;

        event.mCameraDirectLookComponent->mRotRads.x = targetYaw;

        if (m360Progress >= m360Duration) {
            mIsSpinning = false;
            m360Progress = 0.0f;
            m360Duration = 0.0f;
        }
    } else {
        // Оригинальный линейный спин
        float stepDeg = m360Direction * m360Speed.mValue;
        float stepRad = glm::radians(stepDeg);

        event.mCameraDirectLookComponent->mRotRads.x += stepRad;

        while (event.mCameraDirectLookComponent->mRotRads.x > IM_PI)
            event.mCameraDirectLookComponent->mRotRads.x -= 2.0f * IM_PI;
        while (event.mCameraDirectLookComponent->mRotRads.x < -IM_PI)
            event.mCameraDirectLookComponent->mRotRads.x += 2.0f * IM_PI;

        m360CurrentAngle += std::abs(stepDeg);
        if (m360CurrentAngle >= 360.0f) {
            mIsSpinning = false;
            m360CurrentAngle = 0.f;
        }
    }
}

void AutoClicker::onRenderEvent(RenderEvent& event)
{
    // --- BIND PANEL (всегда, без привязки к Hold) ---
    if (mShowBindPanel.mValue) {
        auto drawList = ImGui::GetBackgroundDrawList();
        ImVec2 ss = ImGui::GetIO().DisplaySize;
        float panX = 20.f;
        float panY = ss.y - 120.f;

        std::string btnLabel;
        if (mIsBindingKey)
            btnLabel = mBindWaitRelease ? "Release keys..." : "Press key... (ESC = cancel)";
        else
            btnLabel = std::string("360 Bind: [") + getKeyName(mTriggerKey) + "]";

        ImVec2 labelSize = ImGui::CalcTextSize(btnLabel.c_str());
        float btnW = labelSize.x + 24.f;
        float btnH = labelSize.y + 10.f;

        ImVec2 btnMin{panX, panY};
        ImVec2 btnMax{panX + btnW, panY + btnH};

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool hovered = (mouse.x >= btnMin.x && mouse.x <= btnMax.x && mouse.y >= btnMin.y && mouse.y <= btnMax.y);
        bool clicked = hovered && ImGui::GetIO().MouseClicked[0];

        ImU32 bgColor = mIsBindingKey ? IM_COL32(200, 100, 0, 220)
                                      : (hovered ? IM_COL32(70, 70, 110, 220) : IM_COL32(30, 30, 50, 180));

        drawList->AddRectFilled(btnMin, btnMax, bgColor, 6.f);
        drawList->AddRect(btnMin, btnMax, mIsBindingKey ? IM_COL32(255, 150, 50, 255) : IM_COL32(100, 100, 160, 200), 6.f, 0, 1.5f);
        drawList->AddText({panX + (btnW - labelSize.x) * 0.5f, panY + (btnH - labelSize.y) * 0.5f}, IM_COL32(255, 255, 255, 255), btnLabel.c_str());

        if (clicked && !mIsBindingKey) {
            mIsBindingKey = true;
            mBindWaitRelease = true;
        }
    }

    // --- Стандартный AutoClicker ---
    auto ci = ClientInstance::get();
    auto player = ci->getLocalPlayer();

    if (!player || ci->getScreenName() != "hud_screen") return;

    auto clickGui = gFeatureManager->mModuleManager->getModule<ClickGui>();
    if (clickGui && clickGui->mEnabled) return;

    bool holdActive = true;
    if (mHold.mValue) {
        if (mClickMode.mValue == ClickMode::Both) {
            holdActive = ImGui::IsMouseDown(0) || ImGui::IsMouseDown(1);
        } else {
            holdActive = ImGui::IsMouseDown(mClickMode.as<int>());
        }
    }
    if (mHold.mValue && !holdActive) return;

    auto hitres = player->getLevel()->getHitResult();

    if (mWeaponsOnly.mValue)
    {
        int slot = player->getSupplies()->mSelectedSlot;
        auto item = player->getSupplies()->getContainer()->getItem(slot);
        if (!item->mItem) return;
        auto type = item->getItem()->getItemType();
        if (type != SItemType::Sword) return;
    }

    if (mRandomCPSMin.as<int>() > mRandomCPSMax.as<int>())
        mRandomCPSMin.mValue = mRandomCPSMax.mValue;

    if (mClickMode.mValue == ClickMode::Both)
    {
        bool lmb = ImGui::IsMouseDown(0);
        bool rmb = ImGui::IsMouseDown(1);

        static uint64_t lastAction = 0;
        if (NOW - lastAction < 1000 / mCurrentCPS) goto SKIP_CLICK;
        lastAction = NOW;

        if (lmb)
        {
            if (mAllowBlockBreaking.mValue)
            {
                MouseHook::simulateMouseInput(1, 0, 0, 0, 0, 0);
                MouseHook::simulateMouseInput(1, 1, 0, 0, 0, 0);
            }
            else
            {
                MouseHook::simulateMouseInput(1, 1, 0, 0, 0, 0);
                MouseHook::simulateMouseInput(1, 0, 0, 0, 0, 0);
            }
            tryToolAbuseSwap(player, hitres);
        }

        if (rmb)
        {
            MouseHook::simulateMouseInput(2, 1, 0, 0, 0, 0);
            MouseHook::simulateMouseInput(2, 0, 0, 0, 0, 0);
        }
    }
    else
    {
        int button = mClickMode.as<int>();

        static uint64_t lastAction = 0;
        if (NOW - lastAction < 1000 / mCurrentCPS) goto SKIP_CLICK;
        lastAction = NOW;

        if (button == 0)
        {
            if (mAllowBlockBreaking.mValue)
            {
                MouseHook::simulateMouseInput(1, 0, 0, 0, 0, 0);
                MouseHook::simulateMouseInput(1, 1, 0, 0, 0, 0);
            }
            else
            {
                MouseHook::simulateMouseInput(1, 1, 0, 0, 0, 0);
                MouseHook::simulateMouseInput(1, 0, 0, 0, 0, 0);
            }
            tryToolAbuseSwap(player, hitres);
        }
        else if (button == 1)
        {
            MouseHook::simulateMouseInput(2, 1, 0, 0, 0, 0);
            MouseHook::simulateMouseInput(2, 0, 0, 0, 0, 0);
        }
    }

SKIP_CLICK:
    randomizeCPS();
}