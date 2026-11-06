#pragma once
#include <Features/Modules/Module.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <unordered_map>
#include <deque>

struct PathTracerPoint {
    glm::vec3 position;
    uint64_t timestamp;
};

class PathTracer : public ModuleBase<PathTracer>
{
public:
    NumberSetting mTrailLength = NumberSetting("Trail Length", "Seconds to keep trail", 30.f, 5.f, 120.f, 5.f);
    NumberSetting mSampleRate = NumberSetting("Sample Rate", "Ticks between samples", 4.f, 1.f, 20.f, 1.f);
    NumberSetting mLineWidth = NumberSetting("Line Width", "Trail line thickness", 2.f, 1.f, 5.f, 0.5f);
    BoolSetting mShowSelf = BoolSetting("Show Self", "Show your own trail", true);
    BoolSetting mShowOthers = BoolSetting("Show Others", "Show other players' trails", true);
    BoolSetting mFade = BoolSetting("Fade", "Trails fade over time", true);

    PathTracer() : ModuleBase("PathTracer", "3D trails showing where players walked",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(&mTrailLength, &mSampleRate, &mLineWidth, &mShowSelf, &mShowOthers, &mFade);
        mNames = {
            {Lowercase, "pathtracer"}, {LowercaseSpaced, "path tracer"},
            {Normal, "PathTracer"}, {NormalSpaced, "Path Tracer"}
        };
    }

    void onEnable() override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);
    void onRenderEvent(RenderEvent& event);

private:
    // Map of runtimeID -> trail points
    std::unordered_map<uint64_t, std::deque<PathTracerPoint>> mTrails;
    uint64_t mTickCounter = 0;

    ImColor getColorForEntity(uint64_t runtimeId, int index);
};
