//
// SkinChanger - Replace other players' skins
//

#include "SkinChanger.hpp"

#include <fstream>
#include <filesystem>
#include <Utils/FileUtils.hpp>
#include <Utils/GameUtils/ChatUtils.hpp>

#include "stb_image.h"

bool SkinChanger::loadSkinFromFile(const std::string& path) {
    spdlog::info("[SkinChanger] Loading skin from: {}", path);
    
    if (!std::filesystem::exists(path)) {
        spdlog::error("[SkinChanger] File not found: {}", path);
        ChatUtils::displayClientMessage("§c[SkinChanger] File not found!");
        return false;
    }
    
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        spdlog::error("[SkinChanger] Failed to load: {}", stbi_failure_reason());
        ChatUtils::displayClientMessage("§c[SkinChanger] Failed to load image!");
        return false;
    }
    
    spdlog::info("[SkinChanger] Loaded: {}x{}", width, height);
    
    bool validSize = (width == 64 && height == 32) ||
                     (width == 64 && height == 64) ||
                     (width == 64 && height == 128) ||
                     (width == 128 && height == 128) ||
                     (width == 256 && height == 256);
    
    if (!validSize) {
        ChatUtils::displayClientMessage("§c[SkinChanger] Invalid size: " + std::to_string(width) + "x" + std::to_string(height));
        stbi_image_free(data);
        return false;
    }
    
    size_t dataSize = width * height * 4;
    mCustomSkinData.resize(dataSize);
    memcpy(mCustomSkinData.data(), data, dataSize);
    
    mCustomSkinWidth = width;
    mCustomSkinHeight = height;
    mCurrentSkinPath = path;
    mSkinLoaded = true;
    
    stbi_image_free(data);
    
    ChatUtils::displayClientMessage("§a[SkinChanger] §fLoaded: §b" + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void SkinChanger::onEnable() {
    if (!mSkinLoaded) {
        std::string defaultPath = FileUtils::getSolsticeDir() + "Skins\\custom_skin.png";
        if (std::filesystem::exists(defaultPath)) {
            loadSkinFromFile(defaultPath);
        } else {
            ChatUtils::displayClientMessage("§e[SkinChanger] §fNo skin. Use §b.skin <file>");
        }
    } else {
        ChatUtils::displayClientMessage("§a[SkinChanger] §fEnabled!");
    }
}

void SkinChanger::onDisable() {
    // Ничего не нужно - хук всегда активен, но проверяет mEnabled
}