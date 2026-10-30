#pragma once

//
// Backtrack (визуальный) — показывает, ГДЕ игрок был N мс назад.
//
// Это ровно «визуальная часть» backtrack: призрак модели в отложенной позиции
// плюс след из прошлых позиций. Серверу ничего не отправляется, хитбоксы не
// подменяются, удары идут как обычно — то есть модуль ничего не «взламывает»
// и не может стать причиной рассинхрона.
//
// Зачем это нужно: по призраку видно, насколько игрок «убежал» за время
// задержки, и куда реально смотрел сервер, когда приходит hurt.
//

#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Modules/Module.hpp>

#include <deque>
#include <unordered_map>

class Backtrack : public ModuleBase<Backtrack>
{
public:
    NumberSetting mDelay   = NumberSetting("Delay",        "How far in the past the ghost is drawn (ms)", 200.f, 0.f, 1000.f, 10.f);
    NumberSetting mRange   = NumberSetting("Range",        "Only draw players closer than this (blocks)", 48.f,  8.f, 128.f,  1.f);
    BoolSetting   mTrail   = BoolSetting  ("Trail",        "Draw the path the ghost came from",           true);
    NumberSetting mTrailMs = NumberSetting("Trail Length", "How much of the path to draw (ms)",          500.f, 100.f, 1500.f, 50.f);
    BoolSetting   mLine    = BoolSetting  ("Line",         "Line from the ghost to the current position",  true);

    Backtrack() : ModuleBase("Backtrack", "Visual backtrack: ghost of where players were N ms ago", ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mDelay,
            &mRange,
            &mTrail,
            &mTrailMs,
            &mLine
        );

        VISIBILITY_CONDITION(mTrailMs, mTrail.mValue);

        mNames = {
            {Lowercase,       "backtrack"},
            {LowercaseSpaced, "backtrack"},
            {Normal,          "Backtrack"},
            {NormalSpaced,    "Backtrack"}
        };
    }

    // Одна запись истории: где были ноги игрока и размер его хитбокса.
    struct Sample
    {
        glm::vec3 pos;
        float     width  = 0.6f;
        float     height = 1.8f;
        int64_t   t      = 0;
    };

    static inline std::unordered_map<int64_t, std::deque<Sample>> sHistory;

    static constexpr int64_t kMaxHistoryMs = 2500;  // дольше «прошлого» не храним

    void onEnable()  override;
    void onDisable() override;
    void onBaseTickEvent(BaseTickEvent& event);
    void onRenderEvent  (RenderEvent&   event);

    std::string getSettingDisplay() override
    {
        return std::to_string(static_cast<int>(mDelay.mValue)) + "ms";
    }

private:
    static const Sample* sampleAt(const std::deque<Sample>& history, int64_t wantedT);
    void drawBox(class ImDrawList* dl, const Sample& s, ImU32 col, float thickness);
};
