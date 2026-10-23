#pragma once

#include <Hook/Hook.hpp>
#include <Hook/HookManager.hpp>
#include <SDK/SigManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <Utils/Structs.hpp>

#include <mutex>
#include <queue>
//
// Created by vastrakai on 6/29/2024.
//



class D3DHook : public Hook
{
public:
    D3DHook() : Hook() {
        mName = "D3DHook";
        FrameTransforms = std::make_unique<std::queue<FrameTransform>>();
    }

    static inline std::unique_ptr<std::queue<FrameTransform>> FrameTransforms = nullptr;
    // Очередь пополняет поток игры (ScreenView::setupAndRender), а разбирает
    // поток рендера (Present). Без блокировки одновременные push/pop портили
    // std::deque внутри — случайный краш где угодно, с любыми модулями.
    static inline std::mutex FrameTransformsMutex;
    // Ввод ImGui: Keyboard::feed / MouseDevice::feed (поток ввода) добавляют
    // события в io, а ImGui::NewFrame (поток рендера) их разбирает — тоже
    // только под блокировкой (ImGui не потокобезопасен).
    static inline std::mutex ImGuiInputMutex;
    static inline int transformDelay = 3;
    static inline bool forceFallback = false;

    static bool loadTextureFromEmbeddedResource(const char* resourceName, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
    static bool createTextureFromData(const uint8_t* data, int width, int height, ID3D11ShaderResourceView** out_srv);
    static HRESULT present(IDXGISwapChain3* swapChain, UINT syncInterval, UINT flags);
    static HRESULT resizeBuffers(IDXGISwapChain3* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
    static void initImGui(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    static void shutdownImGui();
    static void igNewFrame();
    static void igEndFrame();
    void init() override;
    static void s_init();
    void shutdown() override;
    static void s_shutdown();
};

