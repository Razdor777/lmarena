#pragma once

#include <cstdint>
#include <Utils/MemUtils.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/Minecraft/Rendering/TexturePtr.hpp>

class ItemStack;
class BaseActorRenderContext;

class ItemRenderer {
public:
    // sub_140CF46C0 — возвращает указатель на внутренний node+0x18.
    // Реальный TexturePtr лежит по адресу (raw + 0x10) = node+0x28.
    // Делай через getIconTexturePtr(), не через getAtlasIconInfoRaw напрямую.
    void* getAtlasIconInfoRaw(ItemStack* stack) {
        using Fn = void* (__fastcall*)(ItemRenderer*, ItemStack*);
        auto fn = reinterpret_cast<Fn>(SigManager::ItemRenderer_getAtlasIconInfo);
        if (!fn) return nullptr;
        return fn(this, stack);
    }

    // Возвращает указатель на TexturePtr для иконки предмета (для плоских предметов).
    // Для блоков обычно nullptr — блоки идут в 3D path через renderGuiItemNew.
    TexturePtr* getIconTexturePtr(ItemStack* stack) {
        void* raw = getAtlasIconInfoRaw(stack);
        if (!raw) return nullptr;
        return reinterpret_cast<TexturePtr*>(reinterpret_cast<char*>(raw) + 0x10);
    }

    // sub_141738690 — возвращает float* на UV (u0, v0, u1, v1).
    static float* getIconUV(ItemStack* stack, int mode = 0) {
        using Fn = float* (__fastcall*)(ItemStack*, unsigned int, unsigned char);
        auto fn = reinterpret_cast<Fn>(SigManager::ItemRenderer_getIconUV);
        if (!fn) return nullptr;
        return fn(stack, (unsigned)mode, (unsigned char)0);
    }

    // sub_140CF8460 — высокоуровневый рендер иконки. Работает и для блоков (3D),
    // но ТРЕБУЕТ валидного BaseActorRenderContext* из текущей рендер-фазы.
    // НЕ зови из ImGui/Present — не будет работать. Пока не хукнули HUD-контекст,
    // этот метод держим на будущее (block icons, glint, 3D предметы).
    void renderGuiItemNew(BaseActorRenderContext* ctx, ItemStack* item, int mode,
                          float x, float y, float opacity, float scale, bool enchanted) {
        using Fn = char(__fastcall*)(
            ItemRenderer*, BaseActorRenderContext*, ItemStack*,
            int mode, float x, float y, bool enchanted,
            int unknown1, float opacity, float scale);
        auto fn = reinterpret_cast<Fn>(SigManager::ItemRenderer_renderGuiItemNew);
        if (!fn) return;
        fn(this, ctx, item, mode, x, y, enchanted, 1, opacity, scale);
    }

    // sub_140CF9BE0 — низкоуровневый icon blit, 16 аргументов (без z в 1.21.44).
    // Оставлен для случаев, когда нужно рисовать иконку напрямую в GameRenderer.
    // В ImGui HUD НЕ применим (нужен BaseActorRenderContext*).
    void iconBlit(BaseActorRenderContext* ctx, TexturePtr* tex,
                  float x, float y, float* uv,
                  float w, float h,
                  float light, float alpha,
                  int color, int secondaryColor,
                  float xscale, float yscale,
                  int glint, bool useMultiColorTextureTinting) {
        using Fn = char(__fastcall*)(
            ItemRenderer*, BaseActorRenderContext*, TexturePtr*,
            float x, float y, float* uv,
            float w, float h,
            float light, float alpha,
            int color, int secondaryColor,
            float xscale, float yscale,
            int glint, char useMultiColorTextureTinting);
        auto fn = reinterpret_cast<Fn>(SigManager::ItemRenderer_iconBlit);
        if (!fn) return;
        fn(this, ctx, tex, x, y, uv, w, h, light, alpha,
           color, secondaryColor, xscale, yscale, glint,
           useMultiColorTextureTinting ? (char)1 : (char)0);
    }
};
