#pragma once

#include <string>
#include <unordered_map>
#include <d3d11.h>

class ItemTextures {
public:
    struct TextureData {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0;
        int height = 0;
    };

    static void init();
    static void shutdown();

    static TextureData* getTexture(const std::string& itemName);
    static TextureData* getTextureForItem(const std::string& fullItemName);

    static bool isReady() { return sInitialized; }

private:
    static inline bool sInitialized = false;
    static inline std::unordered_map<std::string, TextureData> sTextures;
};