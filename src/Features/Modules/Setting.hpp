#pragma once
#include <imgui_internal.h>
#include <imgui.h>
#include <string>
#include <nlohmann/json.hpp>

//
// Setting.hpp — with extra animation fields for ModernDropdown
//

enum class SettingType
{
    Bool,
    Number,
    Enum,
    Color,
    String,
};

class Setting
{
public:
    virtual ~Setting() = default;
    bool parse(const std::string& value);

    std::string mName;
    std::string mDescription;
    SettingType mType;
    bool mDisplay = false;
    bool* mHideOnClickGUI = nullptr;
    std::function<bool()> mIsVisible = []() { return true; };

    Setting(std::string name, std::string description, SettingType type)
        : mName(std::move(name)), mDescription(std::move(description)), mType(type)
    {
    }

    float sliderEase = 0;
    float boolScale  = 0;
    bool  isDragging = false;
    float hoverAnim  = 0.f;

    // Extra animation field for click feedback (used by NumberSetting knob)
    float clickScale = 1.f;

    bool enumExtended = false;
    bool colourExtended = false;
    float enumSlide  = 0;
    float colourSlide = 0;

    virtual nlohmann::json serialize()
    {
        nlohmann::json j;
        j["name"] = mName;
        j["type"] = static_cast<int>(mType);
        return j;
    }
};

#define VISIBILITY_CONDITION(setting, condition) setting.mIsVisible = std::function<bool()>([&]() { return condition; });

class BoolSetting : public Setting
{
public:
    bool mValue = false;
    int mKey;

    BoolSetting(std::string name, std::string description, bool value, int key = -1)
       : Setting(std::move(name), std::move(description), SettingType::Bool), mValue(value), mKey(key)
    {
    }

    void setValue(bool value) { mValue = value; }
    std::string getName() { return mName; }

    nlohmann::json serialize() override
    {
        nlohmann::json j = Setting::serialize();
        j["boolValue"] = mValue;
        j["key"] = mKey;
        return j;
    }

    explicit operator bool() const { return mValue; }
};

class NumberSetting : public Setting
{
public:
    float mValue = 0.0f;
    float mMin = 0.0f;
    float mMax = 0.0f;
    float mStep = 0.0f;

    // Knob bounce animation — triggers when knob hits min/max
    float knobBounce     = 0.f;
    float prevSliderEase = 0.f;

    NumberSetting(std::string name, std::string description, float value, float min, float max, float step)
        : Setting(std::move(name), std::move(description), SettingType::Number),
          mValue(value), mMin(min), mMax(max), mStep(step)
    {
    }

    void setValue(float value)
    {
        mValue = std::round(value / mStep) * mStep;
    }

    nlohmann::json serialize() override
    {
        nlohmann::json j = Setting::serialize();
        j["numberValue"] = mValue;
        return j;
    }

    template <typename T>
    T as() const { return static_cast<T>(mValue); }
};

class EnumSetting : public Setting
{
public:
    int mValue = 0;
    std::vector<std::string> mValues;

    // Per-value hover animation cache (size = mValues.size())
    std::vector<float> enumHoverCache;

    EnumSetting(std::string name, std::string description, int index, std::vector<std::string> values)
        : Setting(std::move(name), std::move(description), SettingType::Enum),
          mValue(index), mValues(std::move(values))
    {
    }

    template <typename IndexType, typename... Args>
    EnumSetting(std::string name, std::string description, IndexType index, Args... values)
        : Setting(std::move(name), std::move(description), SettingType::Enum),
          mValue(static_cast<int>(index))
    {
        mValues = { values... };
    }

    void setValue(int value) { mValue = value; }

    nlohmann::json serialize() override
    {
        nlohmann::json j = Setting::serialize();
        j["enumValue"] = mValue;
        return j;
    }

    template <typename T>
    explicit operator T() const { return static_cast<T>(mValue); }

    template <typename T>
    T as() const { return static_cast<T>(mValue); }
};

template <typename T>
class EnumSettingT : public EnumSetting
{
public:
    struct ValueProxy {
        EnumSetting* owner;
        operator T() const {
            return static_cast<T>(owner->mValue);
        }
        ValueProxy& operator=(T val) {
            owner->mValue = static_cast<int>(val);
            return *this;
        }
    };

    ValueProxy mValue;

    EnumSettingT(std::string name, std::string description, T index, std::vector<std::string> values)
        : EnumSetting(std::move(name), std::move(description), static_cast<int>(index), std::move(values)),
          mValue{this}
    {
    }

    template <typename... Args>
    EnumSettingT(std::string name, std::string description, T index, Args... values)
        : EnumSetting(std::move(name), std::move(description), static_cast<int>(index), values...),
          mValue{this}
    {
    }

    void setValue(T value) { EnumSetting::setValue(static_cast<int>(value)); }

    template <typename type>
    type as() const { return static_cast<type>(EnumSetting::mValue); }
};

class ColorSetting : public Setting
{
public:
    float mValue[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool mIsExtended = false;
    float mSlide = 0;
    static inline float mColorPickerSize = 400;

    ColorSetting(std::string name, std::string description, float r, float g, float b, float a)
        : Setting(std::move(name), std::move(description), SettingType::Color)
    {
        mValue[0] = r; mValue[1] = g; mValue[2] = b; mValue[3] = a;
    }

    ColorSetting(std::string name, std::string description, uint64_t color)
        : Setting(std::move(name), std::move(description), SettingType::Color)
    {
        mValue[0] = ((color >> 16) & 0xFF) / 255.0f;
        mValue[1] = ((color >> 8)  & 0xFF) / 255.0f;
        mValue[2] = (color         & 0xFF) / 255.0f;
        mValue[3] = ((color >> 24) & 0xFF) / 255.0f;
    }

    void setValue(float r, float g, float b, float a) {
        mValue[0] = r; mValue[1] = g; mValue[2] = b; mValue[3] = a;
    }

    ImColor getAsImColor() { return { mValue[0], mValue[1], mValue[2], mValue[3] }; }

    void setFromImColor(const ImColor& color) {
        mValue[0] = color.Value.x; mValue[1] = color.Value.y;
        mValue[2] = color.Value.z; mValue[3] = color.Value.w;
    }

    void setFromImVec4(const ImVec4& color) {
        mValue[0] = color.x; mValue[1] = color.y;
        mValue[2] = color.z; mValue[3] = color.w;
    }

    ImVec4 getAsImVec4() { return { mValue[0], mValue[1], mValue[2], mValue[3] }; }

    void setColor(float r, float g, float b, float a) {
        mValue[0] = r; mValue[1] = g; mValue[2] = b; mValue[3] = a;
    }

    nlohmann::json serialize() override
    {
        nlohmann::json j = Setting::serialize();
        j["colorValue"] = { mValue[0], mValue[1], mValue[2], mValue[3] };
        return j;
    }

    void setFromHex(unsigned long val) {
        mValue[0] = ((val >> 16) & 0xFF) / 255.0f;
        mValue[1] = ((val >> 8)  & 0xFF) / 255.0f;
        mValue[2] = (val         & 0xFF) / 255.0f;
        mValue[3] = ((val >> 24) & 0xFF) / 255.0f;
    }
};
