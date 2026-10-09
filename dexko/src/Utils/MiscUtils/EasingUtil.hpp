#pragma once
#include <cmath>

constexpr float PI = 3.1415927f;

class EasingUtil {
public:
    float percentage = 0.0f;

    EasingUtil() = default;

    __forceinline void resetPercentage() { percentage = 0.0f; }
    __forceinline bool isPercentageMax() const { return percentage >= 1.f; }
    __forceinline bool isPercentageMin() const { return percentage <= 0.f; }
    __forceinline void incrementPercentage(float value) {
        percentage += value;
        if (percentage > 1.f) percentage = 1.f;
    }
    __forceinline void decrementPercentage(float value) {
        percentage -= value;
        if (percentage < 0.f) percentage = 0.f;
    }

    // 🔹 Static pure functions (new API)
    static __forceinline float linear(float t) { return t; }
    static __forceinline float easeInSine(float t) { return std::sin(1.5707963f * t); }
    static __forceinline float easeOutSine(float t) { return 1.f + std::sin(1.5707963f * (t - 1.f)); }
    static __forceinline float easeInOutSine(float t) { return 0.5f * (1.f + std::sin(3.1415926f * (t - 0.5f))); }
    static __forceinline float easeInQuad(float t) { return t * t; }
    static __forceinline float easeOutQuad(float t) { return t * (2.f - t); }
    static __forceinline float easeInOutQuad(float t) { return t < 0.5f ? 2.f * t * t : t * (4.f - 2.f * t) - 1.f; }
    static __forceinline float easeInCubic(float t) { return t * t * t; }
    static __forceinline float easeOutCubic(float t) { return 1.f + (t - 1.f) * (t - 1.f) * (t - 1.f); }
    static __forceinline float easeInOutCubic(float t) { return t < 0.5f ? 4.f * t * t * t : 1.f + (t - 1.f) * (2.f * t - 2.f) * (2.f * t); }
    static __forceinline float easeInQuart(float t) { float t2 = t * t; return t2 * t2; }
    static __forceinline float easeOutQuart(float t) { float t2 = (t - 1.f) * (t - 1.f); return 1.f - t2 * t2; }
    static __forceinline float easeInQuint(float t) { float t2 = t * t; return t * t2 * t2; }
    static __forceinline float easeOutQuint(float t) { float t2 = (t - 1.f) * (t - 1.f); return 1.f + t * t2 * t2; }
    static __forceinline float easeInExpo(float t) { return (std::pow(2.f, 8.f * t) - 1.f) / 255.f; }
    static __forceinline float easeOutExpo(float t) { return 1.f - std::pow(2.f, -8.f * t); }
    static __forceinline float easeInOutExpo(float t) {
        return t < 0.5f ? (std::pow(2.f, 16.f * t) - 1.f) / 510.f : 1.f - 0.5f * std::pow(2.f, -16.f * (t - 0.5f));
    }
    static __forceinline float easeInCirc(float t) { return 1.f - std::sqrt(1.f - t); }
    static __forceinline float easeOutCirc(float t) { return std::sqrt(t); }
    static __forceinline float easeInOutCirc(float t) {
        return t < 0.5f ? (1.f - std::sqrt(1.f - 2.f * t)) * 0.5f : (1.f + std::sqrt(2.f * t - 1.f)) * 0.5f;
    }
    static __forceinline float easeInBack(float t) { return t * t * (2.70158f * t - 1.70158f); }
    static __forceinline float easeOutBack(float t) {
        const float c1 = 1.70158f, c3 = c1 + 1.f;
        return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
    }
    static __forceinline float easeInOutBack(float t) {
        const float c1 = 1.70158f, c2 = c1 * 1.525f;
        return t < 0.5f ? (std::pow(2.f * t, 2.f) * ((c2 + 1.f) * 2.f * t - c2)) / 2.f
                        : (std::pow(2.f * t - 2.f, 2.f) * ((c2 + 1.f) * (t * 2.f - 2.f) + c2) + 2.f) / 2.f;
    }
    static __forceinline float easeInElastic(float t) {
        float t2 = t * t;
        return t2 * t2 * std::sin(t * PI * 4.5f);
    }
    static __forceinline float easeOutElastic(float t) {
        float t2 = (t - 1.f) * (t - 1.f);
        return 1.f - t2 * t2 * std::cos(t * PI * 2.25f);
    }
    static __forceinline float easeInOutElastic(float t) {
        float t2;
        if (t < 0.45f) { t2 = t * t; return 8.f * t2 * t2 * std::sin(t * PI * 9.f); }
        else if (t < 0.55f) { return 0.5f + 0.75f * std::sin(t * PI * 4.f); }
        else { t2 = (t - 1.f) * (t - 1.f); return 1.f - 8.f * t2 * t2 * std::sin(t * PI * 9.f); }
    }
    static __forceinline float easeInBounce(float t) { return std::pow(2.f, 6.f * (t - 1.f)) * std::abs(std::sin(t * PI * 3.5f)); }
    static __forceinline float easeOutBounce(float t) { return 1.f - std::pow(2.f, -6.f * t) * std::abs(std::cos(t * PI * 3.5f)); }
    static __forceinline float easeInOutBounce(float t) {
        return t < 0.5f ? 8.f * std::pow(2.f, 8.f * (t - 1.f)) * std::abs(std::sin(t * PI * 7.f))
                        : 1.f - 8.f * std::pow(2.f, -8.f * t) * std::abs(std::sin(t * PI * 7.f));
    }

    // 🔙 Backward compatibility: instance methods delegating to static ones
    __forceinline float linear() { return linear(percentage); }
    __forceinline float easeInSine() { return easeInSine(percentage); }
    __forceinline float easeOutSine() { return easeOutSine(percentage); }
    __forceinline float easeInOutSine() { return easeInOutSine(percentage); }
    __forceinline float easeInQuad() { return easeInQuad(percentage); }
    __forceinline float easeOutQuad() { return easeOutQuad(percentage); }
    __forceinline float easeInOutQuad() { return easeInOutQuad(percentage); }
    __forceinline float easeInCubic() { return easeInCubic(percentage); }
    __forceinline float easeOutCubic() { return easeOutCubic(percentage); }
    __forceinline float easeInOutCubic() { return easeInOutCubic(percentage); }
    __forceinline float easeInQuart() { return easeInQuart(percentage); }
    __forceinline float easeOutQuart() { return easeOutQuart(percentage); }
    __forceinline float easeInQuint() { return easeInQuint(percentage); }
    __forceinline float easeOutQuint() { return easeOutQuint(percentage); }
    __forceinline float easeInExpo() { return easeInExpo(percentage); }
    __forceinline float easeOutExpo() { return easeOutExpo(percentage); }
    __forceinline float easeInOutExpo() { return easeInOutExpo(percentage); }
    __forceinline float easeInCirc() { return easeInCirc(percentage); }
    __forceinline float easeOutCirc() { return easeOutCirc(percentage); }
    __forceinline float easeInOutCirc() { return easeInOutCirc(percentage); }
    __forceinline float easeInBack() { return easeInBack(percentage); }
    __forceinline float easeOutBack() { return easeOutBack(percentage); }
    __forceinline float easeInOutBack() { return easeInOutBack(percentage); }
    __forceinline float easeInElastic() { return easeInElastic(percentage); }
    __forceinline float easeOutElastic() { return easeOutElastic(percentage); }
    __forceinline float easeInOutElastic() { return easeInOutElastic(percentage); }
    __forceinline float easeInBounce() { return easeInBounce(percentage); }
    __forceinline float easeOutBounce() { return easeOutBounce(percentage); }
    __forceinline float easeInOutBounce() { return easeInOutBounce(percentage); }
};