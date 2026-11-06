//
// Created by vastrakai on 10/19/2024.
//

#pragma once

#include "Event.hpp"

class SetupShaderParameterEvent : public CancelableEvent {
public:
    SetupShaderParameterEvent(
        class ScreenContext* screenContext,
        class BaseActorRenderContext* entityContext,
        class Actor* entity,
        glm::vec4* overlay,
        glm::vec4* changeColor,
        glm::vec4* changeColor2,
        glm::vec4* glintColor,
        int uvOffset1,
        int uvOffset2,
        float uvRot1,
        float uvRot2,
        void** glintUVScale,
        void* uvAnim,
        float br,
        unsigned char lightEmission)
        : mscreenContext(screenContext)
        , mentityContext(entityContext)
        , mentity(entity)
        , moverlay(overlay)
        , mchangeColor(changeColor)
        , mchangeColor2(changeColor2)
        , mglintColor(glintColor)
        , muvOffset1(uvOffset1)
        , muvOffset2(uvOffset2)
        , muvRot1(uvRot1)
        , muvRot2(uvRot2)
        , mglintUVScale(glintUVScale)
        , muvAnim(uvAnim)
        , mbr(br)
        , mlightEmission(lightEmission)
    {}

    class ScreenContext* mscreenContext;
    class BaseActorRenderContext* mentityContext;
    class Actor* mentity;
    glm::vec4* moverlay;
    glm::vec4* mchangeColor;
    glm::vec4* mchangeColor2;
    glm::vec4* mglintColor;
    int muvOffset1;
    int muvOffset2;
    float muvRot1;
    float muvRot2;
    void** mglintUVScale;
    void* muvAnim;
    float mbr;
    unsigned char mlightEmission;
};
