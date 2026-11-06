#include "ItemAtlas.hpp"

#include <unordered_map>
#include <string>
#include <spdlog/spdlog.h>

#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/mce.hpp>                  // ResourceLocation
#include <SDK/Minecraft/Rendering/TexturePtr.hpp> // наш SDK::TexturePtr
#include <SDK/Minecraft/Rendering/ItemRenderer.hpp>
#include <Hook/Hooks/RenderHooks/ItemRendererHook.hpp>
#include <Hook/Hooks/RenderHooks/D3DHook.hpp>
#include <Utils/Resources.hpp>

class BedrockTextureData;

static std::unordered_map<BedrockTextureData*, ID3D11ShaderResourceView*> sNativeCache;
static std::unordered_map<std::string,    ID3D11ShaderResourceView*>       sPathCache;

// ── native path (пока nullptr, ждёт BedrockTextureData) ─────────────────────
ID3D11ShaderResourceView* ItemAtlas::tryExtractNativeSRV(TexturePtr* texPtr)
{
    if (!texPtr) return nullptr;
    BedrockTextureData* data = texPtr->clientTexture.get();
    if (!data) return nullptr;

    auto it = sNativeCache.find(data);
    if (it != sNativeCache.end()) return it->second;

    // TODO: BedrockTextureData -> mce::Texture/bgfx::TextureHandle -> ID3D11ShaderResourceView*.
    // Одна строка заменит этот return, когда будет декомпиляция BedrockTextureData.
    ID3D11ShaderResourceView* srv = nullptr;

    sNativeCache[data] = srv;
    return srv;
}

// ── path fallback: берём PNG по РЕАЛЬНОМУ пути из памяти ────────────────────
static std::string stemFromPath(const std::string& path)
{
    // textures/items/diamond_sword.png -> diamond_sword
    size_t slash = path.find_last_of("/\\");
    std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = file.find_last_of('.');
    return (dot == std::string::npos) ? file : file.substr(0, dot);
}

ID3D11ShaderResourceView* ItemAtlas::tryLoadByPath(const std::string& path)
{
    if (path.empty()) return nullptr;

    auto it = sPathCache.find(path);
    if (it != sPathCache.end()) return it->second;

    // Ключи в Resources — stem'ы. Пробуем несколько вариантов.
    std::string stem  = stemFromPath(path);
    std::string noExt = path;
    size_t dot = noExt.find_last_of('.');
    if (dot != std::string::npos) noExt = noExt.substr(0, dot);

    const char* keys[] = { stem.c_str(), noExt.c_str(), path.c_str() };

    ID3D11ShaderResourceView* srv = nullptr;
    for (const char* key : keys) {
        if (!key || !*key) continue;
        if (ResourceLoader::Resources.find(key) == ResourceLoader::Resources.end()) continue;

        int w = 0, h = 0;
        if (D3DHook::loadTextureFromEmbeddedResource(key, &srv, &w, &h) && srv) break;
    }

    if (!srv) {
        spdlog::debug("[ItemAtlas] path '{}' -> no resource, stem '{}'", path, stem);
    }

    sPathCache[path] = srv;
    return srv;
}

// ── публичный API ───────────────────────────────────────────────────────────
bool ItemAtlas::get(ItemStack* stack, Icon& out)
{
    if (!stack || !stack->mItem) return false;

    Item* item = stack->getItem();
    if (!item) return false;

    ItemRenderer* renderer = ItemRendererHook::getItemRenderer();
    if (!renderer) return false;

    TexturePtr* texPtr = renderer->getIconTexturePtr(stack);
    if (!texPtr) return false;

    // UV всегда доступен, даже для блоков (там правда UV не атласные — но попробуем)
    float* uv = ItemRenderer::getIconUV(stack, 0);

    // 1) пробуем нативный SRV
    if (auto* srv = tryExtractNativeSRV(texPtr)) {
        out.srv = srv;
        out.native = true;
        out.uv0 = uv ? glm::vec2(uv[0], uv[1]) : glm::vec2(0, 0);
        out.uv1 = uv ? glm::vec2(uv[2], uv[3]) : glm::vec2(1, 1);
        return true;
    }

    // 2) фоллбэк по РЕАЛЬНОМУ пути из памяти
    std::string path = getResourcePath(stack);
    if (auto* srv = tryLoadByPath(path)) {
        out.srv = srv;
        out.native = false;
        out.uv0 = glm::vec2(0, 0); // PNG уже нарезан — это целая иконка
        out.uv1 = glm::vec2(1, 1);
        return true;
    }

    return false;
}

std::string ItemAtlas::getResourcePath(ItemStack* stack)
{
    if (!stack) return {};
    ItemRenderer* renderer = ItemRendererHook::getItemRenderer();
    if (!renderer) return {};

    TexturePtr* texPtr = renderer->getIconTexturePtr(stack);
    if (!texPtr) return {};

    auto& loc = texPtr->resourceLocation;
    if (!loc) return {};

    // mce::ResourceLocation — тот же layout, что и SDK
    auto* mceLoc = reinterpret_cast<mce::ResourceLocation*>(loc.get());
    if (!mceLoc) return {};

    return mceLoc->mFilePath;
}

void ItemAtlas::clearCache()
{
    sNativeCache.clear();
    sPathCache.clear();
}
