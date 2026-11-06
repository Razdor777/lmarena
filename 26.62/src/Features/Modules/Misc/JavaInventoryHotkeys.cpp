#include "JavaInventoryHotkeys.hpp"

#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/NetworkItemStackDescriptor.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <Features/FeatureManager.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

// ============================================================
// Key helpers (InfiniteAura style)
// ============================================================

const char* JavaInventoryHotkeys::getKeyName(int vk) {
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

bool JavaInventoryHotkeys::isAnyKeyHeld() {
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

int JavaInventoryHotkeys::findHeldKey() {
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

bool JavaInventoryHotkeys::isKeyJustPressed(int vk) {
    if (vk <= 0) return false;
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool was = mKeyStates[vk];
    mKeyStates[vk] = down;
    return down && !was;
}

// ============================================================
// Item search
// ============================================================

static ItemStack* findUsableItem(int& outSlot, bool& outIsOffhand, bool checkOffhand)
{
    outSlot = -1;
    outIsOffhand = false;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return nullptr;
    auto* inv = player->getSupplies()->getContainer();
    if (!inv) return nullptr;

    const int targets[] = { 368, 322, 259, 344 };

    if (checkOffhand) {
        auto* off = player->getOffhandContainer();
        if (off) {
            ItemStack* it = off->getItem(0);
            if (it && it->mItem) {
                for (int id : targets) {
                    if ((*it->mItem)->mItemId == id) { outSlot = 0; outIsOffhand = true; return it; }
                }
            }
        }
    }

    for (int i = 0; i < 36; i++) {
        ItemStack* it = inv->getItem(i);
        if (!it || !it->mItem) continue;
        for (int id : targets) {
            if ((*it->mItem)->mItemId == id) { outSlot = i; return it; }
        }
    }
    return nullptr;
}

// ============================================================
// Module core
// ============================================================

void JavaInventoryHotkeys::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &JavaInventoryHotkeys::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &JavaInventoryHotkeys::onRenderEvent>(this);
}

void JavaInventoryHotkeys::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &JavaInventoryHotkeys::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &JavaInventoryHotkeys::onRenderEvent>(this);
    resetDesync();
}

nlohmann::json JavaInventoryHotkeys::serializeCustomData() {
    nlohmann::json j;
    j["silentKey"] = mSilentKey;
    j["desyncKey"] = mDesyncKey;
    return j;
}

void JavaInventoryHotkeys::deserializeCustomData(const nlohmann::json& j) {
    if (!j.is_object()) return;
    if (j.contains("silentKey") && j["silentKey"].is_number_integer()) {
        int v = j["silentKey"].get<int>();
        if (v >= 0 && v <= 255) mSilentKey = v;
    }
    if (j.contains("desyncKey") && j["desyncKey"].is_number_integer()) {
        int v = j["desyncKey"].get<int>();
        if (v >= 0 && v <= 255) mDesyncKey = v;
    }
}

void JavaInventoryHotkeys::resetDesync()
{
    mDesyncStage = 0;
    mDesyncFrom = -1;
    mDesyncTo = -1;
}

void JavaInventoryHotkeys::sendUseTxn(int fakeSlot, ItemStack* item, glm::vec3 pos)
{
    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto use = std::make_unique<ItemUseInventoryTransaction>();
    use->mActionType  = ItemUseInventoryTransaction::ActionType::Use;
    use->mTriggerType = ItemUseInventoryTransaction::TriggerType::PlayerInput;
    use->mSlot        = static_cast<uint32_t>(fakeSlot);
    use->mItemInHand  = NetworkItemStackDescriptor(*item);
    use->mPlayerPos   = pos;

    auto* player = ClientInstance::get()->getLocalPlayer();
    if (player) {
        auto* rot = player->getActorRotationComponent();
        if (rot) {
            float yaw   = rot->mYaw   * (3.14159265f / 180.f);
            float pitch = rot->mPitch * (3.14159265f / 180.f);
            glm::vec3 look(
                -cosf(pitch) * sinf(yaw),
                -sinf(pitch),
                 cosf(pitch) * cosf(yaw)
            );
            use->mClickPos = pos + look * 3.0f;
        } else {
            use->mClickPos = pos + glm::vec3(0, 1.62f, 0);
        }
    } else {
        use->mClickPos = pos + glm::vec3(0, 1.62f, 0);
    }

    pkt->mTransaction = std::move(use);
    ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());
}

// -------- Silent Use (UFI) --------
void JavaInventoryHotkeys::doSilentUse()
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;

    int slot = -1;
    bool offhand = false;
    ItemStack* item = findUsableItem(slot, offhand, mOffhandPriority.mValue);
    if (!item) return;

    int current = player->getSupplies()->mSelectedSlot;
    glm::vec3 pos = *player->getPos();

    sendUseTxn(current, item, pos);

    if (offhand)
        spdlog::info("[JIH] Silent offhand use");
    else
        spdlog::info("[JIH] Silent inventory use from slot {}", slot);
}

// -------- Desync Use --------
void JavaInventoryHotkeys::doDesyncPhase1()
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto* inv = player->getSupplies()->getContainer();
    if (!inv) return;

    int slot = -1;
    bool off = false;
    ItemStack* item = findUsableItem(slot, off, false);
    if (!item || off) { mTriggerDesync = false; return; }

    int empty = -1;
    for (int i = 35; i >= 0; i--) {
        ItemStack* it = inv->getItem(i);
        if (!it || !it->mItem) { empty = i; break; }
    }
    if (empty == -1) empty = (slot == 35) ? 34 : 35;

    ItemStack* dst = inv->getItem(empty);
    static ItemStack blank = ItemStack();
    if (!dst) dst = &blank;

    InventoryAction a1(slot, item, dst);
    a1.mSource.mType        = InventorySourceType::ContainerInventory;
    a1.mSource.mContainerId = static_cast<char>(ContainerID::Inventory);

    InventoryAction a2(empty, dst, item);
    a2.mSource.mType        = InventorySourceType::ContainerInventory;
    a2.mSource.mContainerId = static_cast<char>(ContainerID::Inventory);

    auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
    auto cit = std::make_unique<ComplexInventoryTransaction>();
    cit->data.addAction(a1);
    cit->data.addAction(a2);
    pkt->mTransaction = std::move(cit);
    ClientInstance::get()->getPacketSender()->sendToServer(pkt.get());

    mDesyncFrom  = slot;
    mDesyncTo    = empty;
    mDesyncStage = 1;

    spdlog::info("[JIH] Desync phase1: {} -> {}", slot, empty);
}

void JavaInventoryHotkeys::doDesyncPhase2()
{
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) { resetDesync(); return; }
    auto* inv = player->getSupplies()->getContainer();
    if (!inv) { resetDesync(); return; }

    ItemStack* ghost = inv->getItem(mDesyncFrom);
    if (!ghost || !ghost->mItem) {
        resetDesync();
        return;
    }

    int current = player->getSupplies()->mSelectedSlot;
    glm::vec3 pos = *player->getPos();

    sendUseTxn(current, ghost, pos);
    spdlog::info("[JIH] Desync phase2: used ghost item from slot {}", mDesyncFrom);

    resetDesync();
}

// -------- Tick --------
void JavaInventoryHotkeys::onBaseTickEvent(BaseTickEvent& event)
{
    // --- BIND CAPTURE ---
    if (mBindingSilent || mBindingDesync) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            mBindingSilent = false;
            mBindingDesync = false;
            mBindWaitRelease = false;
            return;
        }
        if (mBindWaitRelease) {
            if (!isAnyKeyHeld()) mBindWaitRelease = false;
            return;
        }
        int pressed = findHeldKey();
        if (pressed > 0) {
            if (mBindingSilent) mSilentKey = pressed;
            else mDesyncKey = pressed;
            mBindingSilent = false;
            mBindingDesync = false;
            mBindWaitRelease = false;
            mKeyStates[pressed] = true;
            NotifyUtils::notify(std::string("§aBind set to: §f") + getKeyName(pressed), 2.f, Notification::Type::Info);
        }
        return;
    }

    // --- TRIGGER CHECKS ---
    if (mSilentUse.mValue && mSilentKey > 0 && isKeyJustPressed(mSilentKey))
        mTriggerSilent = true;
    if (mDesyncUse.mValue && mDesyncKey > 0 && isKeyJustPressed(mDesyncKey))
        mTriggerDesync = true;

    // --- ACTIONS ---
    if (mTriggerSilent) {
        mTriggerSilent = false;
        doSilentUse();
    }

    if (mTriggerDesync) {
        mTriggerDesync = false;
        if (mDesyncStage == 0) doDesyncPhase1();
    }

    if (mDesyncStage == 1) {
        doDesyncPhase2();
    }
}

// -------- Render --------
void JavaInventoryHotkeys::renderBindButton(const char* label, bool& binding, int& key,
                                            float& yOffset, float panX, float panY,
                                            ImDrawList* drawList)
{
    std::string btnLabel;
    if (binding)
        btnLabel = std::string(label) + ": Press key... (ESC = cancel)";
    else
        btnLabel = std::string(label) + ": [" + getKeyName(key) + "]";

    ImVec2 labelSize = ImGui::CalcTextSize(btnLabel.c_str());
    float btnW = std::max(labelSize.x + 24.f, 160.f);
    float btnH = labelSize.y + 10.f;

    ImVec2 btnMin{panX, panY + yOffset};
    ImVec2 btnMax{panX + btnW, panY + yOffset + btnH};

    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool hovered = (mouse.x >= btnMin.x && mouse.x <= btnMax.x && mouse.y >= btnMin.y && mouse.y <= btnMax.y);
    bool clicked = hovered && ImGui::GetIO().MouseClicked[0];

    ImU32 bgColor = binding ? IM_COL32(200, 100, 0, 220)
                            : (hovered ? IM_COL32(70, 70, 110, 220) : IM_COL32(30, 30, 50, 180));

    drawList->AddRectFilled(btnMin, btnMax, bgColor, 6.f);
    drawList->AddRect(btnMin, btnMax, binding ? IM_COL32(255, 150, 50, 255) : IM_COL32(100, 100, 160, 200), 6.f, 0, 1.5f);
    drawList->AddText({panX + (btnW - labelSize.x) * 0.5f, panY + yOffset + (btnH - labelSize.y) * 0.5f}, IM_COL32(255, 255, 255, 255), btnLabel.c_str());

    if (clicked && !binding) {
        if (&binding == &mBindingSilent) mBindingDesync = false;
        else mBindingSilent = false;
        binding = true;
        mBindWaitRelease = true;
    }

    yOffset += btnH + 6.f;
}

void JavaInventoryHotkeys::onRenderEvent(RenderEvent& event)
{
    if (!mShowBindPanel.mValue) return;

    auto drawList = ImGui::GetBackgroundDrawList();
    ImVec2 ss = ImGui::GetIO().DisplaySize;

    float panX = 20.f;
    float panY = ss.y - 140.f;
    float offset = 0.f;

    if (mSilentUse.mValue) renderBindButton("Silent", mBindingSilent, mSilentKey, offset, panX, panY, drawList);
    if (mDesyncUse.mValue) renderBindButton("Desync", mBindingDesync, mDesyncKey, offset, panX, panY, drawList);
}