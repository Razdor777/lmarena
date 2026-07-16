#include "Nametags.hpp"

#include <Features/Events/NametagRenderEvent.hpp>
#include <Features/Modules/Misc/Friends.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Options.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/FlagComponent.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/GameUtils/HealthTracker.hpp>
#pragma execution_character_set("utf-8")

// =============================================================
// CLEAN NAME
// =============================================================

std::string Nametags::cleanPlayerName(const std::string& name)
{
    std::string result;
    result.reserve(name.size());
    bool   inBracket = false;
    size_t i         = 0;

    while (i < name.size()) {
        unsigned char c0 = static_cast<unsigned char>(name[i]);

        if (c0 == 0xC2 && i + 1 < name.size() &&
            static_cast<unsigned char>(name[i + 1]) == 0xA7)
        {
            i += 2;
            if (i < name.size()) i++;
            continue;
        }

        if (name[i] == '[') { inBracket = true;  i++; continue; }
        if (name[i] == ']') {
            inBracket = false; i++;
            if (i < name.size() && name[i] == ' ') i++;
            continue;
        }
        if (!inBracket) result += name[i];
        i++;
    }

    size_t start = result.find_first_not_of(' ');
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(' ');
    result = result.substr(start, end - start + 1);

    std::string cleaned;
    cleaned.reserve(result.size());
    bool lastSpace = false;
    for (char c : result) {
        if (c == ' ') { if (!lastSpace) { cleaned += c; lastSpace = true; } }
        else          { cleaned += c;   lastSpace = false; }
    }
    return cleaned;
}

// =============================================================
// ENABLE / DISABLE
// =============================================================

void Nametags::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent,
        &Nametags::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<NametagRenderEvent,
        &Nametags::onNametagRenderEvent>(this);

    mPlayerCache.clear();
    mLastCacheUpdate = 0;

    if (!ItemTextures::isReady())
        ItemTextures::init();
}

void Nametags::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent,
        &Nametags::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<NametagRenderEvent,
        &Nametags::onNametagRenderEvent>(this);

    mPlayerCache.clear();
}

// =============================================================
// CYRILLIC FONT HELPER
// =============================================================

ImFont* Nametags::getCyrillicFont()
{
    static ImFont* cached = nullptr;
    if (cached) return cached;

    static const char* cyrFonts[] = {
        "comfortaa", "open_sans", "product_sans",
        "sf_pro_display", "comfortaa_bold", "open_sans_bold"
    };
    for (auto fn : cyrFonts) {
        auto it = FontHelper::Fonts.find(fn);
        if (it != FontHelper::Fonts.end() && it->second) {
            cached = it->second;
            return cached;
        }
    }
    cached = ImGui::GetFont();
    return cached;
}

// =============================================================
// HELPERS
// =============================================================

ImColor Nametags::getTierColor(const std::string& name)
{
    if (name.find("netherite") != std::string::npos) return ImColor(70,  60,  70,  255);
    if (name.find("diamond")   != std::string::npos) return ImColor(80,  220, 255, 255);
    if (name.find("iron")      != std::string::npos) return ImColor(210, 210, 210, 255);
    if (name.find("golden")    != std::string::npos
     || name.find("gold")      != std::string::npos) return ImColor(255, 215, 50,  255);
    if (name.find("chain")     != std::string::npos) return ImColor(150, 150, 160, 255);
    if (name.find("leather")   != std::string::npos) return ImColor(170, 110, 60,  255);
    if (name.find("turtle")    != std::string::npos) return ImColor(80,  180, 80,  255);
    return ImColor(180, 180, 180, 255);
}

ImColor Nametags::getDurabilityColor(float percent)
{
    percent = std::clamp(percent, 0.f, 1.f);
    if (percent > 0.5f) {
        float t = (percent - 0.5f) * 2.f;
        return ImColor((int)(255 * (1.f - t)), 255, 0, 255);
    }
    float t = percent * 2.f;
    return ImColor(255, (int)(255 * t), 0, 255);
}

std::string Nametags::getShortItemName(const std::string& fullName)
{
    std::string n = fullName;
    size_t colon = n.find(':');
    if (colon != std::string::npos) n = n.substr(colon + 1);

    struct R { const char* from; const char* to; };
    static const R reps[] = {
        {"netherite_","N "}, {"diamond_","D "}, {"iron_","I "},
        {"golden_","G "},    {"chainmail_","C "},{"leather_","L "},
        {"stone_","S "},     {"wooden_","W "},   {"turtle_","T "},
    };
    for (auto& r : reps) {
        if (n.find(r.from) == 0) {
            n = std::string(r.to) + n.substr(strlen(r.from));
            break;
        }
    }

    if (!n.empty()) n[0] = (char)toupper((unsigned char)n[0]);
    for (auto& c : n) if (c == '_') c = ' ';
    return n;
}

std::string Nametags::getShortEnchantString(ItemStack* stack)
{
    if (!stack || !stack->mItem) return "";
    auto enchants = stack->gatherEnchants();
    if (enchants.empty()) return "";

    struct ES { int id; const char* s; };
    static const ES sn[] = {
        {0,"Защ"},   {1,"ОгнЗ"}, {2,"Пад"},  {3,"ВзрЗ"},
        {4,"СнрЗ"},  {5,"Шип"},  {6,"Дых"},  {7,"Глуб"},
        {8,"Вод"},   {9,"Остр"}, {10,"Кара"},{11,"Гиб"},
        {12,"Отбр"}, {13,"Огн"}, {14,"Доб"}, {15,"Эфф"},
        {16,"ШёлК"}, {17,"Прочн"},{18,"Удач"},{19,"Сила"},
        {20,"Откид"},{21,"Пламя"},{22,"Бскн"},{23,"МорУд"},
        {24,"Привл"},{25,"ЛёдХ"},{26,"Почин"},{27,"Прив"},
        {28,"Утрт"}, {29,"Прнз"},{30,"Тягл"},{31,"Верн"},
        {32,"Гром"}, {33,"Мульт"},{34,"Прнк"},{35,"БстрЗ"},
        {36,"ДшСк"}, {37,"БстрК"},
    };

    std::string result;
    result.reserve(64);
    for (auto& [id, lvl] : enchants) {
        if (!result.empty()) result += ' ';
        const char* nm = nullptr;
        for (auto& e : sn) if (e.id == id) { nm = e.s; break; }
        if (nm) result += std::string(nm) + std::to_string(lvl);
        else    result += 'E' + std::to_string(id) + ':' + std::to_string(lvl);
    }
    return result;
}

float Nametags::getFixedDurability(ItemStack* stack)
{
    if (!stack || !stack->mItem) return -1.f;
    Item* item = stack->getItem();
    if (!item || item->mItemId == 0) return -1.f;
    int maxDmg = item->getMaxDamage();
    if (maxDmg <= 0) return -1.f;
    int damage = std::clamp(stack->getDamage() & 0xFFFF, 0, maxDmg);
    return static_cast<float>(maxDmg - damage) / static_cast<float>(maxDmg);
}

// =============================================================
// КЭШ СЛОТА
// =============================================================

void Nametags::buildSlotCache(ItemStack* stack, CachedPlayerInfo::SlotCache& out)
{
    out = {};
    out.isEmpty = true;

    if (!stack || !stack->mItem) return;
    Item* item = stack->getItem();
    if (!item || item->mItemId == 0) return;
    if (stack->mCount <= 0) return;

    out.isEmpty    = false;
    out.itemName   = item->mName;
    out.shortName  = getShortItemName(item->mName);
    out.tierColor  = getTierColor(item->mName);
    out.count      = stack->mCount;
    out.durPercent = getFixedDurability(stack);

    if (stack->mCount > 1)
        out.shortName += " x" + std::to_string(stack->mCount);

    if (mShowEnchants.mValue)
        out.enchants = getShortEnchantString(stack);
}

Nametags::ItemDisplayInfo Nametags::makeDisplayInfo(
    const CachedPlayerInfo::SlotCache& slot) const
{
    ItemDisplayInfo info;
    info.isEmpty    = slot.isEmpty;
    info.itemName   = slot.itemName;
    info.name       = slot.shortName;
    info.enchants   = slot.enchants;
    info.durPercent = slot.durPercent;
    info.tierColor  = slot.tierColor;
    return info;
}

// =============================================================
// ОБНОВЛЕНИЕ КЭША
// =============================================================

void Nametags::updatePlayerCache()
{
    int64_t now = NOW;
    if (now - mLastCacheUpdate < mCacheUpdateDelay) return;
    mLastCacheUpdate = now;

    auto ci = ClientInstance::get();
    if (!ci) return;

    auto* lp = ci->getLocalPlayer();

    auto actors = ActorUtils::getActorList(true, true);

    mPlayerCache.clear();
    mPlayerCache.reserve(actors.size());

    for (auto* actor : actors) {
        if (!actor || !actor->isPlayer())       continue;
        if (!actor->getAABBShapeComponent())    continue;
        if (!actor->getRenderPositionComponent()) continue;

        CachedPlayerInfo info;
        info.actor    = actor;
        info.valid    = true;

        try {
            info.runtimeID = actor->getRuntimeID();
        } catch (...) { continue; }

        try {
            if (actor == lp) {
                std::string n = actor->getNameTag();
                size_t nl = n.find('\n');
                info.displayName = (nl != std::string::npos) ? n.substr(0, nl) : n;
            } else {
                info.displayName = actor->getRawName();
            }
            if (mCleanNames.mValue)
                info.displayName = cleanPlayerName(info.displayName);
            else
                info.displayName = ColorUtils::removeColorCodes(info.displayName);
        } catch (...) {
            info.displayName = "???";
        }

        try {
            info.hp        = actor->getHealth();
            info.maxHp     = actor->getMaxHealth();
            info.absorption = actor->getAbsorption();
        } catch (...) {
            info.hp = info.maxHp = 0.f;
        }

        if (actor != lp && mShowHealth.mValue) {
            try {
                std::string an = actor->getNameTag();
                size_t nl = an.find('\n');
                if (nl != std::string::npos) an = an.substr(0, nl);

                float th, tmh;
                if (HealthTracker::getInstance().getHealth(an, th, tmh)) {
                    info.hp      = th;
                    info.maxHp   = tmh;
                    info.tracked = true;
                    info.stale   = HealthTracker::getInstance().isStale(an, 5000);
                    info.secSince = HealthTracker::getInstance().getSecondsSinceUpdate(an);
                } else if (mCleanNames.mValue) {
                    std::string cn = cleanPlayerName(an);
                    if (HealthTracker::getInstance().getHealth(cn, th, tmh)) {
                        info.hp      = th;
                        info.maxHp   = tmh;
                        info.tracked = true;
                        info.stale   = HealthTracker::getInstance().isStale(cn, 5000);
                        info.secSince = HealthTracker::getInstance().getSecondsSinceUpdate(cn);
                    }
                }
            } catch (...) {}
        }

        try {
            info.isFriend = gFriendManager && gFriendManager->isFriend(actor);
        } catch (...) {}

        if (mShowArmor.mValue) {
            try {
                auto* ac = actor->getArmorContainer();
                if (ac) {
                    for (int i = 0; i < 4; i++) {
                        try { buildSlotCache(ac->getItem(i), info.armor[i]); }
                        catch (...) {}
                    }
                }
            } catch (...) {}
        }

        if (mShowHeldItem.mValue) {
            try {
                auto* sup = actor->getSupplies();
                if (sup) {
                    auto* cont = sup->getContainer();
                    if (cont) {
                        auto* stack = cont->getItem(sup->mSelectedSlot);
                        if (stack && stack->mItem && stack->getItem()->mItemId != 0
                            && stack->mCount > 0)
                        {
                            buildSlotCache(stack, info.mainHand);
                        }
                    }
                }
            } catch (...) {}
        }

        if (mShowOffhand.mValue) {
            try {
                auto* off = actor->getOffhandContainer();
                if (off) {
                    auto* stack = off->getItem(0);
                    if (stack && stack->mItem && stack->getItem()->mItemId != 0
                        && stack->mCount > 0)
                    {
                        if (info.mainHand.isEmpty ||
                            stack->getItem()->mName != info.mainHand.itemName)
                        {
                            buildSlotCache(stack, info.offhand);
                        }
                    }
                }
            } catch (...) {}
        }

        mPlayerCache.push_back(std::move(info));
    }

    auto origin = RenderUtils::transform.mOrigin;
    std::sort(mPlayerCache.begin(), mPlayerCache.end(),
        [&](const CachedPlayerInfo& a, const CachedPlayerInfo& b) {
            if (!a.actor || !b.actor) return false;
            try {
                auto* ap = a.actor->getRenderPositionComponent();
                auto* bp = b.actor->getRenderPositionComponent();
                if (!ap || !bp) return false;
                return glm::distance(origin, ap->mPosition) >
                       glm::distance(origin, bp->mPosition);
            } catch (...) {
                return false;
            }
        });
}

// =============================================================
// RENDER ITEM SLOT
// =============================================================

void Nametags::renderItemSlot(ImDrawList* dl, float x, float y, float size,
    const ItemDisplayInfo& info, bool showDur, bool showEnch)
{
    if (info.isEmpty) return;

    float pad  = 1.f;
    float barH = 2.f;

    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + size, y + size),
        IM_COL32(0, 0, 0, 120), 2.f);
    dl->AddRect(ImVec2(x, y), ImVec2(x + size, y + size),
        info.tierColor, 2.f, 0, 1.f);

    bool texDrawn = false;

    if (mUseTextures.mValue && ItemTextures::isReady()) {
        auto* tex = ItemTextures::getTextureForItem(info.itemName);
        if (tex && tex->srv) {
            float availSize = size - pad * 2.f;
            if (showDur && info.durPercent >= 0.f) availSize -= (barH + 1.f);
            float iconX = x + (size - availSize) / 2.f;
            float iconY = y + pad;
            dl->AddImage((ImTextureID)tex->srv,
                ImVec2(iconX, iconY),
                ImVec2(iconX + availSize, iconY + availSize));
            texDrawn = true;
        }
    }

    if (!texDrawn) {
        ImFont* textFont = getCyrillicFont();
        float   fs       = size * 0.55f;
        ImVec2  ts       = textFont->CalcTextSizeA(fs, FLT_MAX, 0,
                               info.name.c_str());
        float   tx = x + (size - ts.x) / 2.f;
        float   ty = y + (size - ts.y) / 2.f;
        if (showDur && info.durPercent >= 0.f) ty -= barH / 2.f;
        dl->AddText(textFont, fs, ImVec2(tx+1, ty+1),
            IM_COL32(0,0,0,200), info.name.c_str());
        dl->AddText(textFont, fs, ImVec2(tx,   ty),
            info.tierColor,      info.name.c_str());
    }

    if (showDur && info.durPercent >= 0.f) {
        float barY = y + size - barH - pad;
        float barW = size - pad * 2.f;
        float c    = std::clamp(info.durPercent, 0.f, 1.f);
        dl->AddRectFilled(ImVec2(x+pad, barY),
            ImVec2(x+pad+barW,   barY+barH), IM_COL32(40,40,40,200), 1.f);
        dl->AddRectFilled(ImVec2(x+pad, barY),
            ImVec2(x+pad+barW*c, barY+barH), getDurabilityColor(c),   1.f);
    }
}

// =============================================================
// RENDER EQUIPMENT ABOVE NAMETAG
// =============================================================

void Nametags::renderEquipmentAbove(ImDrawList* dl, float centerX, float bottomY,
    float fontSize, const CachedPlayerInfo& info)
{
    if (!mShowArmor.mValue && !mShowHeldItem.mValue && !mShowOffhand.mValue)
        return;

    float baseFS   = mFontSize.mValue > 0.f ? mFontSize.mValue : 23.f;
    float ratio    = fontSize / baseFS;
    float iconSize = std::max(mIconSize.mValue * ratio, 14.f);
    float spacing  = mItemSpacing.mValue;

    float enchScale = mEnchantScale.mValue;
    float enchFS    = std::max(8.f, iconSize * 0.5f * enchScale);

    ImFont* enchantFont = getCyrillicFont();
    ImU32   enchColor   = IM_COL32(
        (int)mEnchantR.mValue,
        (int)mEnchantG.mValue,
        (int)mEnchantB.mValue,
        255);

    struct ItemWithEnch {
        ItemDisplayInfo          disp;
        std::vector<std::string> enchLines;
        float                    maxEnchWidth = 0.f;
    };
    std::vector<ItemWithEnch> items;
    items.reserve(6);

    auto addSlot = [&](const CachedPlayerInfo::SlotCache& slot) {
        if (slot.isEmpty) return;
        ItemWithEnch iwe;
        iwe.disp = makeDisplayInfo(slot);

        if (mShowEnchants.mValue && !slot.enchants.empty()) {
            std::istringstream ss(slot.enchants);
            std::string word;
            while (ss >> word) {
                ImVec2 ts = enchantFont->CalcTextSizeA(
                    enchFS, FLT_MAX, 0, word.c_str());
                if (ts.x > iwe.maxEnchWidth) iwe.maxEnchWidth = ts.x;
                iwe.enchLines.push_back(std::move(word));
            }
        }
        items.push_back(std::move(iwe));
    };

    if (mShowArmor.mValue)
        for (int i = 0; i < 4; i++) addSlot(info.armor[i]);

    if (mShowHeldItem.mValue)
        addSlot(info.mainHand);

    if (mShowOffhand.mValue)
        addSlot(info.offhand);

    if (items.empty()) return;

    std::vector<float> colWidths(items.size());
    for (size_t i = 0; i < items.size(); i++)
        colWidths[i] = std::max(iconSize, items[i].maxEnchWidth + 6.f);

    float totalW = 0.f;
    for (size_t i = 0; i < items.size(); i++) {
        totalW += colWidths[i];
        if (i > 0) totalW += spacing;
    }
    float startX = centerX - totalW / 2.f;

    int maxEnchLines = 0;
    for (auto& it : items)
        maxEnchLines = std::max(maxEnchLines, (int)it.enchLines.size());

    float enchLineH  = enchFS + 3.f;
    float enchBlockH = (float)maxEnchLines * enchLineH;
    float durPctFS   = std::max(8.f, iconSize * 0.45f);
    float durPctH    = mShowDurPercent.mValue ? (durPctFS + 2.f) : 0.f;

    float totalH = 4.f;
    if (mShowEnchants.mValue && maxEnchLines > 0) totalH += enchBlockH + 2.f;
    totalH += iconSize + durPctH;

    float topY = bottomY - totalH;
    float curY = topY;

    if (mShowEnchants.mValue && maxEnchLines > 0) {
        float cx = startX;
        for (size_t i = 0; i < items.size(); i++) {
            float  colW       = colWidths[i];
            float  colCenterX = cx + colW / 2.f;
            auto&  iwe        = items[i];

            for (int ei = 0; ei < (int)iwe.enchLines.size(); ei++) {
                float ey = curY + enchBlockH
                         - (float)(iwe.enchLines.size() - ei) * enchLineH;
                const std::string& line = iwe.enchLines[ei];
                ImVec2 es = enchantFont->CalcTextSizeA(
                    enchFS, FLT_MAX, 0, line.c_str());
                float ex = colCenterX - es.x / 2.f;
                dl->AddText(enchantFont, enchFS,
                    ImVec2(ex+1, ey+1), IM_COL32(0,0,0,200), line.c_str());
                dl->AddText(enchantFont, enchFS,
                    ImVec2(ex,   ey),   enchColor, line.c_str());
            }
            cx += colW + spacing;
        }
        curY += enchBlockH + 2.f;
    }

    {
        float cx = startX;
        for (size_t i = 0; i < items.size(); i++) {
            float colW  = colWidths[i];
            float iconX = cx + (colW - iconSize) / 2.f;
            renderItemSlot(dl, iconX, curY, iconSize,
                items[i].disp, mShowDurability.mValue, false);
            cx += colW + spacing;
        }
        curY += iconSize;
    }

    if (mShowDurPercent.mValue) {
        float cx = startX;
        for (size_t i = 0; i < items.size(); i++) {
            float colW = colWidths[i];
            if (items[i].disp.durPercent >= 0.f) {
                int  pct = (int)(std::clamp(items[i].disp.durPercent, 0.f, 1.f) * 100.f);
                char buf[8];
                snprintf(buf, sizeof(buf), "%d%%", pct);
                ImVec2 ts = enchantFont->CalcTextSizeA(durPctFS, FLT_MAX, 0, buf);
                float  tx = cx + (colW - ts.x) / 2.f;
                ImU32  dc = getDurabilityColor(items[i].disp.durPercent);
                dl->AddText(enchantFont, durPctFS,
                    ImVec2(tx+1, curY+1), IM_COL32(0,0,0,180), buf);
                dl->AddText(enchantFont, durPctFS,
                    ImVec2(tx,   curY),   dc, buf);
            }
            cx += colW + spacing;
        }
    }
}

// =============================================================
// MAIN RENDER — фикс: dangling pointer через runtimeID
// =============================================================

void Nametags::onRenderEvent(RenderEvent& event)
{
    updatePlayerCache();

    auto* ci = ClientInstance::get();
    if (!ci || !ci->getLevelRenderer()) return;

    auto* lp       = ci->getLocalPlayer();
    if (!lp) return;  // ← ФИКС 1: защита от nullptr локального игрока

    auto* drawList = ImGui::GetBackgroundDrawList();
    auto* guiData  = ci->getGuiData();
    if (!guiData) return;

    glm::vec3 origin = RenderUtils::transform.mOrigin;

    for (const auto& cached : mPlayerCache) {
        if (!cached.valid) continue;

        // ← ФИКС 2: резолвим актора по runtimeID, а не используем висячий указатель
        auto* actor = ActorUtils::getActorFromRuntimeID(cached.runtimeID);
        if (!actor) continue;

        // Локальный игрок
        if (actor == lp) {
            if (!mRenderLocal.mValue) continue;
            auto* opts = ci->getOptions();
            if (opts && opts->mThirdPerson->value == 0 &&
                !lp->getFlag<RenderCameraComponent>()) continue;
        }

        // Друг
        if (cached.isFriend) {
            if (!mShowFriends.mValue) continue;
        }

        // Позиция на экране
        glm::vec3 renderPos;
        try {
            renderPos = actor->getRenderPositionComponent()->mPosition;
            if (actor == lp)
                renderPos = RenderUtils::transform.mPlayerPos;
            renderPos.y += 0.5f;
        } catch (...) {
            continue;
        }

        ImVec2 screenIm;
        if (!RenderUtils::worldToScreen(renderPos, screenIm))
            continue;
        glm::vec2 screen(screenIm.x, screenIm.y);
        if (!std::isfinite(screen.x) || !std::isfinite(screen.y)) continue;
        if (screen.x < 0.f || screen.y < 0.f
         || screen.x > guiData->mResolution.x * 2.f
         || screen.y > guiData->mResolution.y * 2.f)
            continue;

        // Размер шрифта
        float fontSize = mFontSize.mValue;
        float padding  = 5.f;

        if (mDistanceScaledFont.mValue) {
            float dist = std::max(0.f,
                glm::distance(origin, renderPos) + 2.5f);
            fontSize = 1.f / dist * 100.f * mScalingMultiplier.mValue;
            fontSize = std::max(fontSize, mMinScale.mValue);
            fontSize = std::max(fontSize, 1.f);
            padding  = fontSize / 4.f;
        }

        FontHelper::pushPrefFont(true);

        if (mStyle.mValue == Style::Advanced) {
            renderAdvancedNametag(cached, drawList, screen, fontSize, padding);
        } else {
            ImColor themeColor = cached.isFriend
                ? ImColor(0.f, 1.f, 0.f, 1.f)
                : ImColor(1.f, 1.f, 1.f, 1.f);

            const std::string& name = cached.displayName;
            auto* font = ImGui::GetFont();

            ImVec2 ifs  = font->CalcTextSizeA(fontSize, FLT_MAX, 0, name.c_str());
            ImVec2 pos  = { screen.x - ifs.x / 2.f, screen.y - ifs.y - 5.f };
            ImVec2 rMin = { pos.x - padding,         pos.y - padding };
            ImVec2 rMax = { pos.x + ifs.x + padding, pos.y + ifs.y + padding };

            if (mBlurStrength.mValue == 0.f)
                drawList->AddRectFilled(rMin, rMax,
                    IM_COL32(0, 0, 0, 127), 10.f);
            ImRenderUtils::addBlur(
                ImVec4(rMin.x, rMin.y, rMax.x, rMax.y),
                mBlurStrength.mValue, 10, drawList, true);
            drawList->AddText(font, fontSize, pos, themeColor, name.c_str());

            renderEquipmentAbove(drawList, screen.x, rMin.y - 2.f,
                fontSize, cached);
        }

        FontHelper::popPrefFont();
    }
}

// =============================================================
// ADVANCED NAMETAG
// =============================================================

void Nametags::renderAdvancedNametag(const CachedPlayerInfo& cached,
    ImDrawList* drawList, const glm::vec2& screen,
    float fontSize, float padding)
{
    float hp         = cached.hp;
    float maxHp      = cached.maxHp;
    float absorption = cached.absorption;
    float hpPct      = (maxHp > 0.f) ? (hp / maxHp) : 0.f;

    const std::string& name = cached.displayName;

    ImVec2 l1s  = ImGui::GetFont()->CalcTextSizeA(
        fontSize, FLT_MAX, 0, name.c_str());

    float  l2fs = fontSize * 0.85f;
    std::string l2;
    bool   hasL2 = false;

    if (mShowHealth.mValue) {
        if (cached.tracked) {
            if (cached.stale)
                l2 = fmt::format("HP: {:.1f}/{:.1f} ({:.0f}s)",
                    hp, maxHp, cached.secSince);
            else
                l2 = fmt::format("HP: {:.1f}/{:.1f}", hp, maxHp);
        } else {
            l2 = "HP: ?/?";
        }
        if (absorption > 0.f)
            l2 += fmt::format(" +{:.1f}", absorption);
        hasL2 = true;
    }

    ImVec2 l2s = hasL2
        ? ImGui::GetFont()->CalcTextSizeA(l2fs, FLT_MAX, 0, l2.c_str())
        : ImVec2(0.f, 0.f);

    float tw  = std::max(l1s.x, l2s.x);
    float th  = l1s.y + (hasL2 ? l2s.y + 2.f : 0.f);
    float sx  = screen.x - tw / 2.f;
    float sy  = screen.y - th - padding * 2.f - 5.f;

    ImVec2 rMin = { sx - padding,      sy - padding };
    ImVec2 rMax = { sx + tw + padding, sy + th + padding };

    float hbH = 3.f;
    if (mShowHealth.mValue) rMax.y += hbH + 2.f;

    if (mBlurStrength.mValue == 0.f)
        drawList->AddRectFilled(rMin, rMax, IM_COL32(0, 0, 0, 140), 6.f);
    ImRenderUtils::addBlur(
        ImVec4(rMin.x, rMin.y, rMax.x, rMax.y),
        mBlurStrength.mValue, 6, drawList, true);
    drawList->AddRect(rMin, rMax, IM_COL32(255, 255, 255, 30), 6.f, 0, 1.f);

    ImU32 nc = cached.isFriend
        ? IM_COL32(0,   255, 0,   255)
        : IM_COL32(255, 255, 255, 255);
    float nameX = screen.x - l1s.x / 2.f;
    drawList->AddText(ImGui::GetFont(), fontSize, {nameX, sy}, nc, name.c_str());

    if (hasL2) {
        float cy  = sy + l1s.y + 2.f;
        float l2x = screen.x - l2s.x / 2.f;
        ImU32 hc;
        if (!cached.tracked) {
            hc = IM_COL32(150, 150, 150, 150);
        } else if (cached.stale) {
            hc = (hpPct > 0.6f) ? IM_COL32(100, 255, 100, 120)
               : (hpPct > 0.3f) ? IM_COL32(255, 255, 100, 120)
                                 : IM_COL32(255, 100, 100, 120);
        } else {
            hc = (hpPct > 0.6f) ? IM_COL32(100, 255, 100, 255)
               : (hpPct > 0.3f) ? IM_COL32(255, 255, 100, 255)
                                 : IM_COL32(255, 100, 100, 255);
        }
        drawList->AddText(ImGui::GetFont(), l2fs, {l2x, cy}, hc, l2.c_str());
    }

    if (mShowHealth.mValue) {
        float ba    = (cached.tracked && !cached.stale) ? 1.f : 0.4f;
        float by    = rMax.y - hbH - 1.f;
        float bxMin = rMin.x + 3.f;
        float bxMax = rMax.x - 3.f;
        float bw    = bxMax - bxMin;

        drawList->AddRectFilled({bxMin, by}, {bxMax, by + hbH},
            IM_COL32(60, 60, 60, 200), 2.f);

        uint8_t baa = (uint8_t)(255 * ba);
        ImU32   bc;
        if (!cached.tracked)      bc = IM_COL32(150, 150, 150, baa);
        else if (hpPct > 0.6f)    bc = IM_COL32(50,  205, 50,  baa);
        else if (hpPct > 0.3f)    bc = IM_COL32(255, 200, 50,  baa);
        else                      bc = IM_COL32(255, 60,  60,  baa);

        drawList->AddRectFilled({bxMin, by},
            {bxMin + bw * hpPct, by + hbH}, bc, 2.f);

        if (absorption > 0.f && maxHp > 0.f) {
            float ap = std::min(absorption / maxHp, 1.f);
            drawList->AddRectFilled(
                {bxMin + bw * hpPct,               by},
                {bxMin + bw * std::min(hpPct + ap, 1.f), by + hbH},
                IM_COL32(255, 215, 0, 200), 2.f);
        }
    }

    renderEquipmentAbove(drawList, screen.x, rMin.y - 2.f, fontSize, cached);
}

// =============================================================
// CANCEL VANILLA NAMETAG — фикс: защита от nullptr
// =============================================================

void Nametags::onNametagRenderEvent(NametagRenderEvent& event)
{
    auto* actor = event.mActor;
    auto* lp    = ClientInstance::get()->getLocalPlayer();
    auto* ci    = ClientInstance::get();

    if (!lp || !ci) return;  // ← ФИКС 3: защита от nullptr

    if (ActorUtils::isBot(actor)) return;
    if (!actor->isPlayer())       return;

    if (actor == lp) {
        auto* opts = ci->getOptions();
        if (opts && opts->mThirdPerson->value == 0
            && !lp->getFlag<RenderCameraComponent>()) return;
    }

    if (actor == lp && !mRenderLocal.mValue) return;
    if (!actor->getAABBShapeComponent()
     || !actor->getRenderPositionComponent()) return;

    event.cancel();
}