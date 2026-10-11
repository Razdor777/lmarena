#pragma once
#include <Hook/Hook.hpp>

class KeyHook : public Hook {
public:
    KeyHook() : Hook() {
        mName = "Keyboard::feed";
    }

    static std::unique_ptr<Detour> mDetour;

    static void onKey(uint32_t key, bool isDown);
    void init() override;
};

