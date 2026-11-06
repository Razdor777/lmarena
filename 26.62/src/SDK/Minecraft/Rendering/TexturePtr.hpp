#pragma once
#include <memory>

class BedrockTextureData;
class ResourceLocation;

// Полный layout TexturePtr, подтверждён декомпиляцией iconBlit (sub_140CF9BE0):
//   +0x00  BedrockTextureData*   // shared_ptr<BedrockTextureData>::_Ptr
//   +0x08  void*                 // shared_ptr<BedrockTextureData>::_Rep
//   +0x10  ResourceLocation*     // shared_ptr<ResourceLocation>::_Ptr
//   +0x18  void*                 // shared_ptr<ResourceLocation>::_Rep
// Размер 0x20.
struct TexturePtr {
    std::shared_ptr<BedrockTextureData> clientTexture;
    std::shared_ptr<ResourceLocation>   resourceLocation;
};

static_assert(sizeof(TexturePtr) == 0x20, "TexturePtr size must be 0x20");
