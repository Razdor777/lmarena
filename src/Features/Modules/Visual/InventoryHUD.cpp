//
// InventoryHUD — инвентарь поверх HUD с иконками.
//

#include "InventoryHUD.hpp"
#include "HudEditor.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/ItemTextures.hpp>
#include <Utils/FontHelper.hpp>

// ─── HUD element ─────────────────────────────────────────────────────────────

static char sInvHUDId[] = "InventoryHUD";

struct InvHUDElement : public HudElement {
    InvHUDElement() : HudElement(sInvHUDId) {
        mVisible  = true;
        mAnchor   = Anchor::TopLeft;
        mPos      = {10.f, 30.f};  // под PlayerHUD по дефолту
        mCentered = false;
    }
};
static InvHUDElement* gInvElem = nullptr;

// ─── Enable / Disable ────────────────────────────────────────────────────────

void InventoryHUD::onEnable() {
    if (!gInvElem) {
        gInvElem = new InvHUDElement();
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gInvElem);
    }
    gInvElem->mVisible = true;

    if (mUseTextures.mValue && !ItemTextures::isReady())
        ItemTextures::init();

    gFeatureManager->mDispatcher->listen<RenderEvent, &InventoryHUD::onRenderEvent>(this);
}

void InventoryHUD::onDisable() {
    if (gInvElem) gInvElem->mVisible = false;
    gFeatureManager->mDispatcher->deafen<RenderEvent, &InventoryHUD::onRenderEvent>(this);
}

// ─── Draw one slot ───────────────────────────────────────────────────────────

void InventoryHUD::drawSlot(ImDrawList* dl, float x, float y, float size,
                             ItemStack* stack, bool selected, float opacity, float rounding)
{
    // Фон слота
    ImColor bgCol = selected
        ? ImColor(60, 60, 90, (int)(240 * opacity))
        : ImColor(20, 20, 30, (int)(200 * opacity));

    dl->AddRectFilled({x, y}, {x+size, y+size}, bgCol, rounding);

    // Обводка — белая если выбран, тёмная иначе
    ImColor borderCol = selected
        ? ImColor(200, 200, 255, (int)(220 * opacity))
        : ImColor(60, 60, 80, (int)(160 * opacity));
    dl->AddRect({x, y}, {x+size, y+size}, borderCol, rounding, 0, selected ? 1.5f : 1.f);

    if (!stack || !stack->mItem) return;

    Item* item = stack->getItem();
    if (!item) return;

    std::string itemName = item->mName;
    float iconSize = size - 6.f;
    float ix = x + 3.f;
    float iy = y + 3.f;

    // Текстура
    bool texDrawn = false;
    if (mUseTextures.mValue && ItemTextures::isReady()) {
        auto* tex = ItemTextures::getTextureForItem(itemName);
        if (tex && tex->srv) {
            dl->AddImage((ImTextureID)tex->srv,
                {ix, iy}, {ix + iconSize, iy + iconSize});
            texDrawn = true;
        }
    }

    // Фоллбэк — первые 2 буквы названия
    if (!texDrawn && !itemName.empty()) {
        std::string abbr = itemName.substr(0, 2);
        // Ищем материал по имени для цвета
        ImColor matCol = ImColor(200, 200, 200, 240);
        if (itemName.find("diamond")   != std::string::npos) matCol = ImColor(80,  220, 255, 240);
        if (itemName.find("netherite") != std::string::npos) matCol = ImColor(160, 120, 200, 240);
        if (itemName.find("gold")      != std::string::npos) matCol = ImColor(255, 210, 50,  240);
        if (itemName.find("iron")      != std::string::npos) matCol = ImColor(200, 200, 200, 240);

        float fs = size * 0.35f;
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0, abbr.c_str());
        float tx = x + (size - ts.x) * 0.5f;
        float ty = y + (size - ts.y) * 0.5f;
        dl->AddText(ImGui::GetFont(), fs, {tx+1,ty+1}, ImColor(0,0,0,150), abbr.c_str());
        dl->AddText(ImGui::GetFont(), fs, {tx, ty},    matCol,              abbr.c_str());
    }

    // Количество стака (правый нижний угол)
    if (mShowCount.mValue && stack->mCount > 1) {
        char cnt[8]; snprintf(cnt, sizeof(cnt), "%d", (int)stack->mCount);
        float fs = size * 0.28f;
        if (fs < 7.f) fs = 7.f;
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0, cnt);
        float tx = x + size - ts.x - 2.f;
        float ty = y + size - ts.y - 1.f;
        // Тень
        dl->AddText(ImGui::GetFont(), fs, {tx+1.f, ty+1.f}, ImColor(0,0,0,200), cnt);
        dl->AddText(ImGui::GetFont(), fs, {tx, ty},          ImColor(255,255,255,240), cnt);
    }
}

// ─── Render ──────────────────────────────────────────────────────────────────

void InventoryHUD::onRenderEvent(RenderEvent& event) {
    auto* player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    if (!gInvElem || !gInvElem->mVisible) return;

    // Только в hud_screen если настройка включена
    if (mOnlyInGame.mValue) {
        std::string scr = ClientInstance::get()->getScreenName();
        if (scr != "hud_screen") return;
    }

    if (mUseTextures.mValue && !ItemTextures::isReady())
        ItemTextures::init();

    auto* supplies = player->getSupplies();
    if (!supplies) return;
    auto* container = supplies->getContainer();
    if (!container) return;

    auto* armorCont   = player->getArmorContainer();
    auto* offhandCont = player->getOffhandContainer();

    auto* dl  = ImGui::GetBackgroundDrawList();
    float sc  = mScale.mValue;
    float ss  = mSlotSize.mValue * sc;   // размер одного слота
    float gap = 2.f * sc;                // зазор между слотами
    float op  = mOpacity.mValue;
    float rd  = mRounding.mValue * sc;
    int   sel = supplies->mSelectedSlot;

    FontHelper::pushPrefFont(false);

    ImVec2 anchor = gInvElem->getPos();
    float x0 = anchor.x;
    float y  = anchor.y;
    float rowW = 9.f * (ss + gap) - gap;  // ширина строки из 9 слотов

    // ── Заголовок ────────────────────────────────────────────────────────
    if (mShowTitle.mValue) {
        float titleH = 18.f * sc;
        dl->AddRectFilled({x0, y}, {x0 + rowW, y + titleH},
            ImColor(15, 15, 25, (int)(220 * op)), rd);
        // Цветная линия сверху
        float now = (float)ImGui::GetTime();
        ImColor lineCol = ColorUtils::getThemedColor(now * 20.f);
        dl->AddRectFilled({x0 + rd*0.4f, y}, {x0 + rowW - rd*0.4f, y + 1.5f}, lineCol);

        float fs = 10.f * sc;
        const char* title = "Inventory";
        ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0, title);
        float tx = x0 + 6.f * sc;
        float ty = y + (titleH - ts.y) * 0.5f;
        dl->AddText(ImGui::GetFont(), fs, {tx+1,ty+1}, ImColor(0,0,0,100), title);
        dl->AddText(ImGui::GetFont(), fs, {tx, ty},
            ImColor(200, 200, 220, 240), title);
        y += titleH + gap;
    }

    // ── Броня (4 слота вертикально слева, если включено) ─────────────────
    float armorColWidth = 0.f;
    if (mShowArmor.mValue && armorCont) {
        armorColWidth = ss + gap;
        float ax = x0;
        float ay = y;
        // Слоты брони: 3=шлем, 2=нагрудник, 1=поножи, 0=ботинки
        for (int i = 3; i >= 0; i--) {
            auto* st = armorCont->getItem(i);
            drawSlot(dl, ax, ay, ss, st, false, op, rd);
            ay += ss + gap;
        }
    }

    float invX = x0 + armorColWidth;

    // ── Инвентарь (ряды 1-3, слоты 9-35) ────────────────────────────────
    if (mShowInventory.mValue) {
        for (int row = 0; row < 3; row++) {
            float rx = invX;
            float ry = y + row * (ss + gap);
            for (int col = 0; col < 9; col++) {
                int idx = 9 + row * 9 + col;
                auto* st = container->getItem(idx);
                drawSlot(dl, rx, ry, ss, st, false, op, rd);
                rx += ss + gap;
            }
        }
        y += 3.f * (ss + gap);
    }

    // ── Хотбар (слоты 0-8) ───────────────────────────────────────────────
    if (mShowHotbar.mValue) {
        float rx = invX;
        for (int i = 0; i < 9; i++) {
            auto* st = container->getItem(i);
            bool  hi = mHighlightSel.mValue && (i == sel);
            drawSlot(dl, rx, y, ss, st, hi, op, rd);
            rx += ss + gap;
        }

        // Оффхенд справа от хотбара
        if (mShowOffhand.mValue && offhandCont) {
            float ox = invX + 9.f * (ss + gap) + gap * 2.f;
            auto* st = offhandCont->getItem(0);
            drawSlot(dl, ox, y, ss, st, false, op, rd);
        }

        y += ss + gap;
    }

    // Обновляем размер HUD element
    gInvElem->mSize = {rowW + armorColWidth, y - anchor.y};

    ImGui::PopFont();
}
