#include "ArmorHUD.hpp"
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/SimpleContainer.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <imgui/imgui.h>
#include "HudEditor.hpp"

static char sArmorHudIdentifier[] = "ArmorHUD";

class ArmorHudElement : public HudElement {
public:
    ArmorHudElement() : HudElement(sArmorHudIdentifier) {
        mVisible = true; mCentered = false; mAnchor = Anchor::TopLeft;
    }
};

static ArmorHudElement* gArmorHudElement = nullptr;

void ArmorHUD::onEnable() {
    // УБРАЛИ ItemTextures::init() отсюда
    gFeatureManager->mDispatcher->listen<RenderEvent, &ArmorHUD::onRenderEvent>(this);
    if (!gArmorHudElement) {
        gArmorHudElement = new ArmorHudElement();
        gArmorHudElement->mSize = { 200, 50 };
        if (HudEditor::gInstance) HudEditor::gInstance->registerElement(gArmorHudElement);
    }
    if (gArmorHudElement) gArmorHudElement->mVisible = true;
}

void ArmorHUD::onDisable() {
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ArmorHUD::onRenderEvent>(this);
    if (gArmorHudElement) gArmorHudElement->mVisible = false;
}

float ArmorHUD::getFixedDurability(ItemStack* stack) {
    if (!stack || !stack->mItem) return -1.f;
    
    Item* item = stack->getItem();
    int maxDmg = item->getMaxDamage();
    
    if (maxDmg <= 0) return -1.f;
    
    int damage = 0;
    if (stack->mCompoundTag) {
        auto* variant = stack->mCompoundTag->get("Damage");
        if (variant) damage = variant->asIntTag()->val & 0xFFFF;
    }
    
    return std::clamp(static_cast<float>(maxDmg - damage) / static_cast<float>(maxDmg), 0.f, 1.f);
}

ImColor ArmorHUD::getDurabilityColor(float percent) {
    percent = std::clamp(percent, 0.f, 1.f);
    if (percent > 0.5f) {
        float t = (percent - 0.5f) * 2.f;
        return ImColor((int)(255 * (1.f - t)), 255, 0, 255);
    }
    float t = percent * 2.f;
    return ImColor(255, (int)(255 * t), 0, 255);
}

ImColor ArmorHUD::getMaterialColor(const std::string& name) {
    if (name.find("netherite") != std::string::npos) return ImColor(70, 60, 70, 255);
    if (name.find("diamond")   != std::string::npos) return ImColor(80, 220, 255, 255);
    if (name.find("iron")      != std::string::npos) return ImColor(210, 210, 210, 255);
    if (name.find("golden") != std::string::npos ||
        name.find("gold")   != std::string::npos)    return ImColor(255, 215, 50, 255);
    if (name.find("chain")     != std::string::npos) return ImColor(150, 150, 160, 255);
    if (name.find("leather")   != std::string::npos) return ImColor(170, 110, 60, 255);
    if (name.find("turtle")    != std::string::npos) return ImColor(80, 180, 80, 255);
    return ImColor(180, 180, 180, 255);
}

std::string ArmorHUD::getShortName(const std::string& fullName) {
    std::string n = fullName;
    size_t colon = n.find(':');
    if (colon != std::string::npos) n = n.substr(colon + 1);
    struct R { const char* from; const char* to; };
    static const R reps[] = {
        {"netherite_","N "}, {"diamond_","D "}, {"iron_","I "},
        {"golden_","G "}, {"chainmail_","C "}, {"leather_","L "},
        {"stone_","S "}, {"wooden_","W "}, {"turtle_","T "},
    };
    for (auto& r : reps)
        if (n.find(r.from) == 0) { n = std::string(r.to) + n.substr(strlen(r.from)); break; }
    if (!n.empty()) n[0] = toupper(n[0]);
    for (auto& c : n) if (c == '_') c = ' ';
    return n;
}

ArmorHUD::SlotRenderInfo ArmorHUD::getSlotInfo(ItemStack* stack, const char* label) {
    SlotRenderInfo info;
    info.label = label;
    info.valid = false;
    info.durPercent = -1.f;
    info.materialColor = ImColor(180, 180, 180, 255);

    if (!stack || !stack->mItem) return info;
    Item* item = stack->getItem();
    if (!item) return info;

    info.valid = true;
    info.shortName = getShortName(item->mName);
    info.materialColor = getMaterialColor(item->mName);
    info.durPercent = getFixedDurability(stack);

    return info;
}

void ArmorHUD::renderSlot(ImDrawList* dl, float x, float y, float slotSize,
                           const SlotRenderInfo& info, const std::string& itemName)
{
    float scale = mScale.mValue;
    float padding = 4.f * scale;
    float barHeight = 4.f * scale;

    // Background
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + slotSize, y + slotSize),
        ImColor(0, 0, 0, static_cast<int>(255 * mOpacity.mValue)), 4.f * scale);
    dl->AddRect(ImVec2(x, y), ImVec2(x + slotSize, y + slotSize),
        info.materialColor, 4.f * scale, 0, 1.5f);

    bool textureDrawn = false;

    // Текстура — ЦЕНТРИРОВАНИЕ ФИКС
    if (mUseTextures.mValue && ItemTextures::isReady()) {
        auto* tex = ItemTextures::getTextureForItem(itemName);
        if (tex && tex->srv) {
            // Доступная область внутри слота
            float availSize = slotSize - padding * 2;
            if (mShowBar.mValue && info.durPercent >= 0.f)
                availSize -= (barHeight + 2.f);

            // Центрируем иконку
            float iconX = x + (slotSize - availSize) / 2.f;
            float iconY = y + padding;

            dl->AddImage((ImTextureID)tex->srv,
                ImVec2(iconX, iconY),
                ImVec2(iconX + availSize, iconY + availSize));
            textureDrawn = true;
        }
    }

    // Текст-фоллбэк
    if (!textureDrawn) {
        if (mShowName.mValue && !info.shortName.empty()) {
            float maxW = slotSize - padding * 2;
            float fs = ImGui::GetFontSize();
            ImVec2 ts = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0, info.shortName.c_str());
            float nameScale = std::min(1.f, maxW / ts.x);
            float actualFS = fs * nameScale;
            ImVec2 as = ImGui::GetFont()->CalcTextSizeA(actualFS, FLT_MAX, 0, info.shortName.c_str());
            float tx = x + (slotSize - as.x) / 2;
            float ty = y + padding;
            dl->AddText(ImGui::GetFont(), actualFS, ImVec2(tx+1,ty+1), ImColor(0,0,0,200), info.shortName.c_str());
            dl->AddText(ImGui::GetFont(), actualFS, ImVec2(tx,ty), info.materialColor, info.shortName.c_str());
        } else {
            float fs = 14.f * scale * 1.5f;
            ImVec2 ls = ImGui::GetFont()->CalcTextSizeA(fs, FLT_MAX, 0, info.label.c_str());
            float lx = x + (slotSize - ls.x) / 2;
            float ly = y + (slotSize - ls.y) / 2;
            dl->AddText(ImGui::GetFont(), fs, ImVec2(lx+1,ly+1), ImColor(0,0,0,200), info.label.c_str());
            dl->AddText(ImGui::GetFont(), fs, ImVec2(lx,ly), info.materialColor, info.label.c_str());
        }
    }

    // Durability bar
    if (mShowBar.mValue && info.durPercent >= 0.f) {
        float barY = y + slotSize - barHeight - padding;
        float barW = slotSize - padding * 2;
        float clamped = std::clamp(info.durPercent, 0.f, 1.f);
        dl->AddRectFilled(ImVec2(x+padding, barY), ImVec2(x+padding+barW, barY+barHeight),
            ImColor(40,40,40,200), 2.f);
        dl->AddRectFilled(ImVec2(x+padding, barY), ImVec2(x+padding+barW*clamped, barY+barHeight),
            getDurabilityColor(clamped), 2.f);
    }
}

void ArmorHUD::onRenderEvent(RenderEvent& event)
{
    // Отложенная инициализация текстур (D3D должен быть готов)
    if (mUseTextures.mValue && !ItemTextures::isReady()) {
        ItemTextures::init();
    }
    
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto armorContainer = player->getArmorContainer();
    if (!armorContainer) return;
    auto drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    ImVec2 basePos;
    if (gArmorHudElement && gArmorHudElement->mVisible) {
        basePos = gArmorHudElement->getPos();
        gArmorHudElement->mSampleMode = false;
    } else {
        basePos = ImVec2(mPosX.mValue, mPosY.mValue);
    }

    float scale = mScale.mValue;
    float slotSize = 40.f * scale;
    float spacing = 5.f * scale;
    float percentH = 15.f * scale;

    struct SlotData { SlotRenderInfo info; std::string itemName; };
    std::vector<SlotData> slots;

    static const char* armorLabels[] = {"Helm", "Chest", "Legs", "Boots"};
    for (int i = 0; i < 4; i++) {
        TRY_CALL([&]() {
            ItemStack* stack = armorContainer->getItem(i);
            auto si = getSlotInfo(stack, armorLabels[i]);
            if (si.valid) {
                std::string iname = (stack && stack->getItem())
                    ? stack->getItem()->mName : "";
                slots.push_back({si, iname});
            }
        });
    }

    if (mShowMainHand.mValue) {
        auto supplies = player->getSupplies();
        if (supplies) {
            auto container = supplies->getContainer();
            if (container) {
                TRY_CALL([&]() {
                    ItemStack* stack = container->getItem(supplies->mSelectedSlot);
                    auto si = getSlotInfo(stack, "Hand");
                    if (si.valid && si.durPercent >= 0.f) {
                        std::string iname = (stack && stack->getItem())
                            ? stack->getItem()->mName : "";
                        slots.push_back({si, iname});
                    }
                });
            }
        }
    }

    if (mShowOffhand.mValue) {
        auto offhand = player->getOffhandContainer();
        if (offhand) {
            TRY_CALL([&]() {
                ItemStack* stack = offhand->getItem(0);
                auto si = getSlotInfo(stack, "Off");
                if (si.valid) {
                    std::string iname = (stack && stack->getItem())
                        ? stack->getItem()->mName : "";
                    slots.push_back({si, iname});
                }
            });
        }
    }

    if (slots.empty()) return;

    int rendered = 0;
    for (auto& sd : slots) {
        float x, y;
        float extra = (mShowPercent.mValue ? percentH : 0.f);

        if (mHorizontal.mValue) {
            x = basePos.x + rendered * (slotSize + spacing);
            y = basePos.y;
        } else {
            x = basePos.x;
            y = basePos.y + rendered * (slotSize + spacing + extra);
        }

        renderSlot(drawList, x, y, slotSize, sd.info, sd.itemName);

        // Процент снизу
        if (mShowPercent.mValue && sd.info.durPercent >= 0.f) {
            float clamped = std::clamp(sd.info.durPercent, 0.f, 1.f);
            char pctText[16];
            snprintf(pctText, sizeof(pctText), "%d%%", (int)(clamped * 100.f));
            float pctFS = ImGui::GetFontSize() * scale;
            ImVec2 ps = ImGui::GetFont()->CalcTextSizeA(pctFS, FLT_MAX, 0, pctText);
            float px = x + (slotSize - ps.x) / 2;
            float py = y + slotSize + 2.f * scale;
            drawList->AddText(ImGui::GetFont(), pctFS, ImVec2(px+1,py+1), ImColor(0,0,0,200), pctText);
            drawList->AddText(ImGui::GetFont(), pctFS, ImVec2(px,py), getDurabilityColor(clamped), pctText);
        }
        rendered++;
    }

    // Update HUD element size
    if (gArmorHudElement) {
        float extra = mShowPercent.mValue ? percentH : 0.f;
        if (mHorizontal.mValue)
            gArmorHudElement->mSize = { rendered * (slotSize + spacing), slotSize + extra };
        else
            gArmorHudElement->mSize = { slotSize, rendered * (slotSize + spacing + extra) };
    }
}