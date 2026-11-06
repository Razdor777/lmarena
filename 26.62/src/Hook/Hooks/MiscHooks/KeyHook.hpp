#pragma once
#include <Hook/Hook.hpp>
//
// Created by vastrakai on 6/25/2024.
//


class KeyHook : public Hook {
public:
    KeyHook() : Hook() {
        mName = "Keyboard::feed";
    }

    static std::unique_ptr<Detour> mDetour;

    // true — клавишу погасить (игра её не увидит). См. комментарий в .cpp.
    static bool onKey(uint32_t key, bool isDown);
    void init() override;
};

