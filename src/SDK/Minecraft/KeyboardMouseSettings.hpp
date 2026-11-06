#pragma once
//
// Created by vastrakai on 10/10/2024.
//


class MinecraftBind
{
public:
    std::string mBindName;
    std::vector<int> mBindKey;
    PAD(0x8);
};


// 26.62 renamed the binds holder: ClientInputMappingFactory now exposes
// getKeyboardRemappingLayout() and the type is KeyboardRemappingLayout.
// The layout itself is unchanged, so the old name is kept as an alias and every
// module written against KeyboardMouseSettings keeps compiling.
class KeyboardRemappingLayout {
public:
    PAD(0x8);
    std::vector<MinecraftBind> mKeyTypeA;
    std::vector<MinecraftBind> mKeyTypeB;

    int operator[](const std::string& key);
};

using KeyboardMouseSettings = KeyboardRemappingLayout;

class ClientInputMappingFactory {
public:
    class KeyboardRemappingLayout* getKeyboardRemappingLayout();

    // thin compatibility wrapper for existing call sites
    class KeyboardRemappingLayout* getKeyboardMouseSettings() {
        return getKeyboardRemappingLayout();
    }
};

class ClientInputHandler {
public:
    class ClientInputMappingFactory* getMappingFactory();
};