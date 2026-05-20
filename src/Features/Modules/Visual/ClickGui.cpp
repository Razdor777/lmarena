#include "ClickGui.hpp"
#include <Features/Events/MouseEvent.hpp>
#include <Features/Events/KeyEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/MinecraftGame.hpp>

void ClickGui::onEnable()
{
    auto ci=ClientInstance::get();
    if(!ci) return;
    if(!mLastMouseState){
        auto mc=ci->getMinecraftGame();
        mWasMouseGrabbed=mc?mc->getMouseGrabbed():false;
        mLastMouseState=true;
        ci->releaseMouse();
    }
    gFeatureManager->mDispatcher->listen<MouseEvent,&ClickGui::onMouseEvent>(this);
    gFeatureManager->mDispatcher->listen<KeyEvent,&ClickGui::onKeyEvent,nes::event_priority::FIRST>(this);
}

void ClickGui::onDisable()
{
    gFeatureManager->mDispatcher->deafen<MouseEvent,&ClickGui::onMouseEvent>(this);
    gFeatureManager->mDispatcher->deafen<KeyEvent,&ClickGui::onKeyEvent>(this);
    auto ci=ClientInstance::get();
    if(ci&&mWasMouseGrabbed&&mLastMouseState) ci->grabMouse();
    mLastMouseState=false;
    mWasMouseGrabbed=false;
}

void ClickGui::onWindowResizeEvent(WindowResizeEvent& event)
{
    mModernGui.onWindowResizeEvent(event);
}

void ClickGui::onMouseEvent(MouseEvent& event) {
    if (mEnabled) {
        if (event.mActionButtonId != 4) {  // 4 = wheel in MouseHook, don't cancel wheel
            event.mCancelled = true;
        }
    }
}

void ClickGui::onKeyEvent(KeyEvent& event)
{
    if(!mEnabled) return;

    if((event.mKey==this->mKey||event.mKey==VK_ESCAPE)&&event.mPressed){
        if(mIsAnimatingClose){
            event.mCancelled=true;
            return;
        }
        if(!mModernGui.isBinding&&!mModernGui.displayColorPicker){
            this->toggle();
        }
        event.mCancelled=true;
        return;
    }

    if(mModernGui.isBinding){
        event.mCancelled=true;
        return;
    }

    if(event.mKey==VK_SHIFT){
        mIsPressingShift=event.mPressed;
    }

    event.mCancelled=true;
}

float ClickGui::getEaseAnim(EasingUtil ease, int mode)
{
    switch(mode){
    case 0: return mEnabled ? ease.easeOutExpo() : ease.easeInQuad();
    case 1: return mEnabled ? ease.easeOutElastic() : ease.easeInBack();
    default: return ease.easeOutExpo();
    }
}

void ClickGui::onRenderEvent(RenderEvent& event)
{
    static float animation=0;
    static EasingUtil inEase=EasingUtil();
    static EasingUtil blurEase=EasingUtil();

    float delta=ImGui::GetIO().DeltaTime;
    float speed=mEaseSpeed.mValue/10.f;

    mEnabled?blurEase.incrementPercentage(delta*speed*1.5f)
            :blurEase.decrementPercentage(delta*speed*3.f);
    mEnabled?inEase.incrementPercentage(delta*speed)
            :inEase.decrementPercentage(delta*speed*2.f);

    float inScale = getEaseAnim(inEase, mAnimation.as<int>());
    inScale=MathUtils::clamp(inScale,0.0f,1.0f);

    float blurAnim=MathUtils::clamp(mEnabled?blurEase.easeOutExpo():blurEase.easeOutQuad(),0.f,1.f);
    animation=blurAnim;

    mIsAnimatingClose = (!mEnabled && (blurAnim > 0.0001f || inScale > 0.0001f));

    if(animation<0.0001f) return;

    int scrollDirection=0;
    float wheel=ImGui::GetIO().MouseWheel;
    if(wheel>0.01f) scrollDirection=-1;
    else if(wheel<-0.01f) scrollDirection=1;

    if(mStyle.mValue==ClickGuiStyle::Modern){
        mModernGui.render(animation,inScale,scrollDirection,
                         mBlurStrength.mValue*blurAnim,mMidclickRounding.mValue,mIsPressingShift);
    }
}