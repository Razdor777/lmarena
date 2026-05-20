#include "Arraylist.hpp"
#include <Features/FeatureManager.hpp>
#include <Features/Modules/Visual/Interface.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

void Arraylist::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent, &Arraylist::onRenderEvent>(this);
}

void Arraylist::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &Arraylist::onRenderEvent>(this);
    for (auto& mod : gFeatureManager->mModuleManager->getModules()) {
        mod->mArrayListAnim = 0.f;
        mod->pos.y = -999.f;
    }
}

struct ALItem {
    Module*     mod      = nullptr;
    std::string name;
    std::string suffix;
    float nameW  = 0.f;
    float suffixW= 0.f;
    float totalW = 0.f;
    ImColor color;
    float colorOffset = 0.f;
    float y      = -999.f;
    float targetY= 0.f;
};

void Arraylist::onRenderEvent(RenderEvent& event)
{
    float delta = ImGui::GetIO().DeltaTime;
    ImVec2 displayRes = ImGui::GetIO().DisplaySize;
    auto drawList  = ImGui::GetBackgroundDrawList();

    auto daInterface = gFeatureManager->mModuleManager->getModule<Interface>();
    if (!daInterface) return;

    FontHelper::pushPrefFont(false, mBoldText.mValue, false);

    float scale      = mScale.mValue;
    const float fontSize   = mFontSize.mValue * scale * 0.64f;
    const float suffixSize = fontSize * 0.72f;
    const float lineH      = fontSize * 1.18f;
    const float rightMargin= 8.f * scale;
    const float accentW    = 2.5f * scale;
    const float maxW       = 270.f * scale;

    auto& modules = gFeatureManager->mModuleManager->getModules();

    // ── Animate ────────────────────────────────────────────────────────────
    for (auto& mod : modules) {
        if (!mod->mVisibleInArrayList.mValue) continue;
        if (mVisibility.mValue == ModuleVisibility::Bound && mod->mKey == 0) continue;

        float target = mod->mEnabled ? 1.f : 0.f;
        float speed  = target > mod->mArrayListAnim ? mEnableAnimSpeed.mValue : mDisableAnimSpeed.mValue;
        mod->mArrayListAnim = MathUtils::lerp(mod->mArrayListAnim, target, delta * speed);
        mod->mArrayListAnim = MathUtils::clamp(mod->mArrayListAnim, 0.f, 1.f);
    }

    // ── Collect active items ─────────────────────────────────────────────────
    std::vector<ALItem> items;
    int activeIndex = 0;

    for (auto& mod : modules) {
        if (!mod->mVisibleInArrayList.mValue) continue;
        if (mVisibility.mValue == ModuleVisibility::Bound && mod->mKey == 0) continue;
        if (mod->mArrayListAnim < 0.005f) continue;

        std::string name   = mod->getName();
        std::string suffix = mod->getSettingDisplayText();
        if (!mRenderMode.mValue) suffix = "";

        float nameW   = ImGui::GetFont()->CalcTextSizeA(fontSize,   FLT_MAX, 0, name.c_str()).x;
        float suffixW = suffix.empty() ? 0.f : ImGui::GetFont()->CalcTextSizeA(suffixSize, FLT_MAX, 0, suffix.c_str()).x;
        float totalW  = std::min(nameW + (suffix.empty() ? 0.f : 5.f + suffixW), maxW);

        float colorOffset = (float)activeIndex * 42.f;
        ImColor color = ColorUtils::getThemedColor(colorOffset);

        ALItem item;
        item.mod          = mod.get();
        item.name         = name;
        item.suffix       = suffix;
        item.nameW        = nameW;
        item.suffixW      = suffixW;
        item.totalW       = totalW;
        item.color        = color;
        item.colorOffset  = colorOffset;
        item.y            = mod->pos.y;
        items.push_back(item);
        activeIndex++;
    }

    // ── Sort by width ───────────────────────────────────────────────────────
    std::sort(items.begin(), items.end(), [](const ALItem& a, const ALItem& b) {
        return a.totalW > b.totalW;
    });

    // ── Y layout ─────────────────────────────────────────────────────────────
    float currentY = 4.f;
    for (size_t i = 0; i < items.size(); i++) {
        items[i].targetY = currentY;
        if (items[i].y < -900.f) items[i].y = items[i].targetY;
        items[i].y = MathUtils::lerp(items[i].y, items[i].targetY, delta * 14.f);
        if (std::abs(items[i].y - items[i].targetY) < 0.2f) items[i].y = items[i].targetY;
        currentY += lineH * items[i].mod->mArrayListAnim;
    }

    // ── X with elastic/slide ─────────────────────────────────────────────────
    for (auto& item : items) {
        float baseX   = displayRes.x - rightMargin - item.totalW - accentW - 6.f;
        float hiddenX = displayRes.x + 60.f;
        float anim    = item.mod->mArrayListAnim;

        float staggerDelay = mStaggerAnim.mValue ? (&item - items.data()) * 0.03f : 0.f;
        if (mStaggerAnim.mValue && staggerDelay > 0.f) {
            float raw = MathUtils::clamp((anim - staggerDelay) / 0.5f, 0.f, 1.f);
            EasingUtil e; e.percentage = raw; anim = e.easeOutExpo();
        }

        float x;
        if (mElasticAnim.mValue && item.mod->mEnabled && anim < 1.f) {
            EasingUtil e; e.percentage = anim; x = MathUtils::lerp(hiddenX, baseX, e.easeOutElastic());
        } else {
            EasingUtil e; e.percentage = anim; x = MathUtils::lerp(hiddenX, baseX, e.easeOutExpo());
        }
        item.mod->pos.x = x;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // RENDER — text + accent strip only, NO background, NO dots, NO lines
    // ═══════════════════════════════════════════════════════════════════════
    for (auto& item : items)
    {
        float alpha = item.mod->mArrayListAnim;
        if (alpha < 0.01f) continue;

        float x = item.mod->pos.x;
        float y = item.y;
        float w = item.totalW;
        float h = lineH;

        // ── ACCENT STRIP only ────────────────────────────────────────────────
        float stripX = x + w + 2.f;
        ImColor stripCol = item.color; stripCol.Value.w = alpha;
        drawList->AddRectFilled({stripX, y + 2.f}, {stripX + accentW, y + h - 2.f}, stripCol, accentW * 0.5f);

        // ── GLOW behind strip (optional) ─────────────────────────────────────
        if (mGlow.mValue) {
            ImColor gc = item.color; gc.Value.w = alpha * 0.72f;
            ImRenderUtils::fillShadowRectangle(
                ImVec4(stripX, y + 2.f, stripX + accentW, y + h - 2.f),
                gc, alpha, mGlowStrength.mValue * 55.f * alpha, 0, accentW * 0.5f, drawList);
        }

        // ── TEXT ──────────────────────────────────────────────────────────────
        ImColor nameCol = item.color; nameCol.Value.w = alpha;
        drawList->AddText(ImGui::GetFont(), fontSize, {x, y}, nameCol, item.name.c_str());

        // ── SUFFIX ────────────────────────────────────────────────────────────
        if (!item.suffix.empty()) {
            ImColor sc(0.58f, 0.58f, 0.62f, alpha * 0.85f);
            drawList->AddText(ImGui::GetFont(), suffixSize,
                {x + item.nameW + 5.f, y + (h - ImGui::GetFont()->CalcTextSizeA(suffixSize, FLT_MAX, 0, item.suffix.c_str()).y) * 0.5f},
                sc, item.suffix.c_str());
        }

        // Click toggle
        ImVec4 hitRect(x - 3.f, y - 1.f, x + w + accentW + 6.f, y + h + 1.f);
        bool hovered = ImGui::GetIO().MousePos.x >= hitRect.x && ImGui::GetIO().MousePos.x <= hitRect.z &&
                       ImGui::GetIO().MousePos.y >= hitRect.y && ImGui::GetIO().MousePos.y <= hitRect.w;
        if (hovered && ImGui::IsMouseClicked(0))
            item.mod->toggle();

        item.mod->pos.y = item.y;
    }

    ImGui::PopFont();
}