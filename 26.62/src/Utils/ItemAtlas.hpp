#pragma once
#include <d3d11.h>
#include <glm/glm.hpp>
#include <string>

class ItemStack;
struct TexturePtr;

class ItemAtlas {
public:
    struct Icon {
        ID3D11ShaderResourceView* srv = nullptr; // borrowed (кэш владеет)
        glm::vec2 uv0{0.0f, 0.0f};
        glm::vec2 uv1{1.0f, 1.0f};
        bool      native = false;                // true = из BedrockTextureData, false = из PNG по пути
    };

    static bool get(ItemStack* stack, Icon& out);
    static void clearCache();

    // Для дебага: получить путь из TexturePtr без обращения к BedrockTextureData.
    // Возвращает "" если путь недоступен.
    static std::string getResourcePath(ItemStack* stack);

private:
    static ID3D11ShaderResourceView* tryExtractNativeSRV(TexturePtr* texPtr);
    static ID3D11ShaderResourceView* tryLoadByPath(const std::string& path);
};
