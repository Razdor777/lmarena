//
// Created by vastrakai on 6/29/2024.
//

#include "ClickGui.hpp"

#include <Features/Events/MouseEvent.hpp>
#include <Features/Events/KeyEvent.hpp>
#include <Features/GUI/ModernDropdown.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>

static ModernGui modernGui = ModernGui();


void ClickGui::onEnable()
{
    // getMouseGrabbed() инвертирован (!getMinecraftGame()->getMouseGrabbed())
    // поэтому инвертируем обратно чтобы получить реальное состояние
    mWasGrabbed = !ClientInstance::get()->getMouseGrabbed();
    mGuiOpen = true;
    ClientInstance::get()->releaseMouse();

    gFeatureManager->mDispatcher->listen<MouseEvent, &ClickGui::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<KeyEvent, &ClickGui::onKeyEvent, nes::event_priority::FIRST>(this);
}

void ClickGui::onDisable()
{
    gFeatureManager->mDispatcher->deafen<MouseEvent, &ClickGui::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<KeyEvent, &ClickGui::onKeyEvent>(this);

    // 3. GUI закрыт — СНАЧАЛА снимаю флаг, ПОТОМ возвращаю состояние
    mGuiOpen = false;

    if (mWasGrabbed)
        ClientInstance::get()->grabMouse();   // была захвачена → снова захвачена (невидима)
    // else: была видима → ничего не делаю, курсор и так видимый
}

void ClickGui::onWindowResizeEvent(WindowResizeEvent& event)
{
    modernGui.onWindowResizeEvent(event);
}

void ClickGui::onMouseEvent(MouseEvent& event)
{
    event.mCancelled = true;
}

void ClickGui::onKeyEvent(KeyEvent& event)
{
    if (event.mKey == VK_ESCAPE) {
        if (!modernGui.isBinding && event.mPressed) this->toggle();
        event.mCancelled = true;
        return;
    }

    if (modernGui.isBinding) {
        event.mCancelled = true;
        return;
    }

    if (modernGui.mSearching && event.mPressed)
    {
        int key = event.mKey;
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (key == VK_BACK)
        {
            int len = (int)strlen(modernGui.mSearchBuffer);
            if (len > 0)
                modernGui.mSearchBuffer[len - 1] = '\0';
        }
        else
        {
            char c = 0;
            if (key >= 'A' && key <= 'Z')
                c = shift ? (char)key : (char)(key + 32);
            else if (key >= '0' && key <= '9')
                c = (char)key;
            else if (key == VK_SPACE)
                c = ' ';
            else if (key == VK_OEM_MINUS)
                c = shift ? '_' : '-';
            else if (key == VK_OEM_PLUS)
                c = shift ? '+' : '=';

            if (c != 0)
            {
                int len = (int)strlen(modernGui.mSearchBuffer);
                if (len < (int)sizeof(modernGui.mSearchBuffer) - 1)
                {
                    modernGui.mSearchBuffer[len] = c;
                    modernGui.mSearchBuffer[len + 1] = '\0';
                }
            }
        }

        event.mCancelled = true;
        return;
    }

    if (event.mKey == VK_SHIFT && event.mPressed) {
        mIsPressingShift = true;
        event.mCancelled = true;
    } else {
        mIsPressingShift = false;
    }
}

float ClickGui::getEaseAnim(EasingUtil ease, int mode)
{
    switch (mode) {
    case 0: return ease.easeOutExpo();
    case 1: return mEnabled ? ease.easeOutElastic() : ease.easeOutBack();
    default: return ease.easeOutExpo();
    }
}

void ClickGui::onRenderEvent(RenderEvent& event)
{
    // Пока GUI открыт — держу курсор видимым (игра пытается вернуть каждый тик)
    // После закрытия (mGuiOpen=false) — НЕ трогаю мышку вообще
    if (mGuiOpen)
        ClientInstance::get()->releaseMouse();

    static float animation = 0;
    static int scrollDirection = 0;
    static EasingUtil inEase = EasingUtil();

    float delta = ImGui::GetIO().DeltaTime;

    mEnabled
        ? inEase.incrementPercentage(delta * mEaseSpeed.mValue / 10)
        : inEase.decrementPercentage(delta * 2 * mEaseSpeed.mValue / 10);

    float inScale = getEaseAnim(inEase, mAnimation.as<int>());
    if (inEase.isPercentageMax()) inScale = 0.996;
    if (mAnimation.mValue == ClickGuiAnimation::Zoom)
        inScale = MathUtils::clamp(inScale, 0.0f, 0.996);

    animation = MathUtils::lerp(0, 1, inEase.easeOutExpo());

    if (animation < 0.0001f) return;

    if (ImGui::GetIO().MouseWheel > 0)
        scrollDirection = -1;
    else if (ImGui::GetIO().MouseWheel < 0)
        scrollDirection = 1;
    else
        scrollDirection = 0;

    if (mStyle.mValue == ClickGuiStyle::Modern)
    {
        modernGui.render(animation, inScale, scrollDirection,
            mBlurStrength.mValue, mMidclickRounding.mValue, mIsPressingShift);
    }
}