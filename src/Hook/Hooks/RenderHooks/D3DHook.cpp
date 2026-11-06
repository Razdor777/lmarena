//
// Created by vastrakai on 6/29/2024.
//

#include "D3DHook.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <Solstice.hpp>
#include <kiero.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <Features/Events/WindowResizeEvent.hpp>
#include <SDK/Minecraft/Rendering/bgfx_context.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/ProcUtils.hpp>
#include <Utils/Resource.hpp>
#include <Utils/Resources.hpp>
#include <Utils/MiscUtils/D2D.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <winrt/base.h>

using winrt::com_ptr;

static decltype(&D3DHook::present) oPresent = nullptr;
static decltype(&D3DHook::resizeBuffers) oResizeBuffers = nullptr;
static decltype(&D3DHook::resizeBuffers1) oResizeBuffers1 = nullptr;

static com_ptr<ID3D11Device> gDevice11;
static com_ptr<ID3D11DeviceContext> gContext11;
static com_ptr<ID3D11DeviceContext> gDevice_context11;
static com_ptr<ID3D12Device> gDevice12;
static com_ptr<ID3D11On12Device> gDevice11on12;
static std::vector<com_ptr<ID3D11Resource>> mBackBuffer11Tex;
static std::vector<com_ptr<ID3D11RenderTargetView>> mBackBuffer11Rtv;

static bool imGuiInitialized = false;
static ID3D12CommandQueue* realQueue = nullptr;

static HWND wnd = NULL;
static IDXGISwapChain* gSwapChain = nullptr;

#define CHECK_HR(hr) if (FAILED(hr)) { spdlog::error("HRESULT failed: {0}", hr); return hr; }

static bool alreadyRunningD3D11 = false;
static bool d3dInitImGui = false;

struct LoadedResource {
    ID3D11ShaderResourceView* srv;
    int width;
    int height;
};

static std::unordered_map<std::string, LoadedResource> gLoadedResources;

bool D3DHook::loadTextureFromEmbeddedResource(const char* resourceName, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    if (!gDevice11) return false;

    if (gLoadedResources.contains(resourceName))
    {
        LoadedResource& resource = gLoadedResources[resourceName];
        *out_srv = resource.srv;
        *out_width = resource.width;
        *out_height = resource.height;
        return true;
    }

    Resource resource = ResourceLoader::Resources[resourceName];
    if (resource.data() == nullptr)
    {
        spdlog::error("Failed to load embedded resource: {0}", resourceName);
        return false;
    }

    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory(static_cast<stbi_uc const*>(resource.data2()), resource.size(), &image_width, &image_height, nullptr, 4);
    if (image_data == NULL)
    {
        spdlog::error("Failed to load embedded image: {0}", resourceName);
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    HRESULT hr = gDevice11->CreateTexture2D(&desc, &subResource, &pTexture);
    if (FAILED(hr) || !pTexture)
    {
        stbi_image_free(image_data);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    gDevice11->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    gLoadedResources[resourceName] = { *out_srv, image_width, image_height };

    return true;
}

bool D3DHook::createTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    if (!filename || !out_srv || !out_width || !out_height) {
        spdlog::error("Invalid argument(s) provided to CreateTextureFromFile.");
        return false;
    }

    int width, height, channels;
    uint8_t* data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        spdlog::error("Failed to load image from file: {0}", filename);
        return false;
    }

    if (!createTextureFromData(data, width, height, out_srv)) {
        spdlog::error("Failed to create texture from data.");
        stbi_image_free(data);
        return false;
    }

    stbi_image_free(data);

    *out_width = width;
    *out_height = height;

    return true;
}

bool D3DHook::createTextureFromData(const uint8_t* data, int width, int height, ID3D11ShaderResourceView** out_srv)
{
    ID3D11Device* device = gDevice11.get();

    if (!device || !data || width <= 0 || height <= 0) {
        spdlog::error("Invalid argument(s) provided to CreateTextureFromData.");
        return false;
    }

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = static_cast<UINT>(width);
    textureDesc.Height = static_cast<UINT>(height);
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = static_cast<UINT>(width * 4);
    initData.SysMemSlicePitch = 0;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&textureDesc, &initData, &texture);
    if (FAILED(hr) || !texture) {
        spdlog::error("Failed to create texture from data.");
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(texture, &srvDesc, out_srv);
    texture->Release();
    if (FAILED(hr)) {
        spdlog::error("Failed to create shader resource view.");
        return false;
    }

    return true;
}

static void releaseSwapChainTargets()
{
    if (!d3dInitImGui) return;

    if (gContext11)
    {
        ID3D11RenderTargetView* nullTargets[1] = { nullptr };
        gContext11->OMSetRenderTargets(1, nullTargets, nullptr);
        gContext11->Flush();
    }

    D2D::shutdown();
    D3DHook::shutdownImGui();

    mBackBuffer11Rtv.clear();
    mBackBuffer11Tex.clear();

    d3dInitImGui = false;
}

HRESULT D3DHook::present(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    if (Solstice::mRequestEject || Solstice::mUnloading.load())
    {
        return oPresent(swapChain, syncInterval, flags);
    }

    gSwapChain = swapChain;

    static bool once = false;
    if (!once) {
        if (swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&gDevice11)) == S_OK) {
            spdlog::info("[D3D] D3D11 Device acquired");
            alreadyRunningD3D11 = true;

            static bool msgwarn = false;
            if (!msgwarn)
            {
                NotifyUtils::notify("WARNING! Running in D3D11 mode.\nD3D12 is recommended for best stability.", 10.f, Notification::Type::Warning);
                msgwarn = true;
            }
        }

        if (swapChain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&gDevice12)) == S_OK) {
            spdlog::info("[D3D] D3D12 Device acquired");

            if (Solstice::Prefs && Solstice::Prefs->mFallbackToD3D11)
            {
                ID3D12Device* bad_device = nullptr;
                if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&bad_device))))
                {
                    ID3D12Device5* dev5 = nullptr;
                    if (SUCCEEDED(bad_device->QueryInterface(IID_PPV_ARGS(&dev5))))
                    {
                        dev5->RemoveDevice();
                        dev5->Release();
                    }
                    bad_device->Release();
                    return oPresent(swapChain, syncInterval, flags);
                }
            }

            UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;
            auto* bgfxCtx = bgfx_context::get();
            ID3D12CommandQueue* pacq = (bgfxCtx && bgfxCtx->getRenderContext())
                ? reinterpret_cast<bgfx_d3d12_RendererContextD3D12*>(bgfxCtx->getRenderContext())->getCommandQueue()
                : nullptr;

            if (pacq)
            {
                CHECK_HR(pacq->QueryInterface(IID_PPV_ARGS(&realQueue)));

                CHECK_HR(D3D11On12CreateDevice(
                    gDevice12.get(),
                    deviceFlags,
                    nullptr,
                    0,
                    reinterpret_cast<IUnknown**>(&pacq),
                    1,
                    0,
                    gDevice11.put(),
                    gDevice_context11.put(),
                    nullptr
                ));
            }
        }

        once = true;
    }

    if (forceFallback && !alreadyRunningD3D11)
    {
        spdlog::warn("Forcing fallback to D3D11");
        releaseSwapChainTargets();

        if (gDevice11on12)
        {
            gDevice11on12->Release();
            gDevice11on12 = nullptr;
        }

        ID3D12Device* bad_device = nullptr;
        if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&bad_device))))
        {
            ID3D12Device5* dev5 = nullptr;
            if (SUCCEEDED(bad_device->QueryInterface(IID_PPV_ARGS(&dev5))))
            {
                dev5->RemoveDevice();
                dev5->Release();
            }
            bad_device->Release();
            forceFallback = false;
            once = false;
            return oPresent(swapChain, syncInterval, flags);
        }
        forceFallback = false;
    }

    while (FrameTransforms && FrameTransforms->size() > static_cast<size_t>(transformDelay)) {
        RenderUtils::transform = FrameTransforms->front();
        FrameTransforms->pop();
    }

    if (!d3dInitImGui)
    {
        if (!gDevice11) return oPresent(swapChain, syncInterval, flags);

        DXGI_SWAP_CHAIN_DESC sdesc{};
        swapChain->GetDesc(&sdesc);
        if (sdesc.OutputWindow && !Solstice::GameWindow)
            Solstice::GameWindow = sdesc.OutputWindow;
        wnd = sdesc.OutputWindow ? sdesc.OutputWindow : ProcUtils::getMinecraftWindow();

        int count = alreadyRunningD3D11 ? 1 : (sdesc.BufferCount > 0 ? static_cast<int>(sdesc.BufferCount) : 3);
        mBackBuffer11Tex.resize(count);
        mBackBuffer11Rtv.resize(count);

        if (alreadyRunningD3D11) {
            gDevice11->GetImmediateContext(gContext11.put());
            CHECK_HR(swapChain->GetBuffer(0, IID_PPV_ARGS(mBackBuffer11Tex.at(0).put())));
            CHECK_HR(gDevice11->CreateRenderTargetView(mBackBuffer11Tex.at(0).get(), NULL, mBackBuffer11Rtv.at(0).put()));
        } else {
            gContext11 = gDevice_context11;

            D3D11_RESOURCE_FLAGS backBuffer11Flags = {};
            backBuffer11Flags.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            CHECK_HR(gDevice11.try_as(gDevice11on12));
            for (int i = 0; i < count; i++) {
                com_ptr<ID3D12Resource> backBuffer12;
                CHECK_HR(swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffer12.put())));
                CHECK_HR(gDevice11on12->CreateWrappedResource(
                    backBuffer12.get(), &backBuffer11Flags,
                    D3D12_RESOURCE_STATE_PRESENT,
                    D3D12_RESOURCE_STATE_PRESENT,
                    IID_PPV_ARGS(mBackBuffer11Tex.at(i).put())));

                D3D11_TEXTURE2D_DESC texDesc;
                reinterpret_cast<ID3D11Texture2D*>(mBackBuffer11Tex.at(i).get())->GetDesc(&texDesc);

                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.ViewDimension = texDesc.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Format = sdesc.BufferDesc.Format;
                rtvDesc.Texture2D.MipSlice = 0;

                CHECK_HR(gDevice11->CreateRenderTargetView(mBackBuffer11Tex.at(i).get(), nullptr, mBackBuffer11Rtv.at(i).put()));
            }
        }
        d3dInitImGui = true;
    }

    float dpi = wnd ? static_cast<float>(GetDpiForWindow(wnd)) : 96.0f;

    UINT index = 0;
    if (!alreadyRunningD3D11)
    {
        winrt::com_ptr<IDXGISwapChain3> swapChain3;
        HRESULT hr = swapChain->QueryInterface(IID_PPV_ARGS(swapChain3.put()));
        if (FAILED(hr)) {
            return oPresent(swapChain, syncInterval, flags);
        }
        index = swapChain3->GetCurrentBackBufferIndex();
        if (index >= mBackBuffer11Tex.size()) {
            return oPresent(swapChain, syncInterval, flags);
        }
    }

    initImGui(gDevice11.get(), gContext11.get());
    if (!imGuiInitialized)
        return oPresent(swapChain, syncInterval, flags);

    D2D::init(swapChain, gDevice11.get());

    static ImVec2 lastWindowSize = ImGui::GetIO().DisplaySize;
    auto* ci = ClientInstance::get();
    auto* guiData = ci ? ci->getGuiData() : nullptr;
    if (guiData)
    {
        ImVec2 windowSize = ImVec2(guiData->mResolution.x, guiData->mResolution.y);
        if (lastWindowSize.x != windowSize.x || lastWindowSize.y != windowSize.y) {
            auto holder = nes::make_holder<WindowResizeEvent>(windowSize.x, windowSize.y);
            gFeatureManager->mDispatcher->trigger(holder);
        }
        lastWindowSize = windowSize;
    }

    winrt::com_ptr<IDXGISurface> surface;
    mBackBuffer11Tex.at(index).try_as(surface);
    D2D::beginRender(surface.get(), dpi);

    igNewFrame();

    auto holder = nes::make_holder<RenderEvent>();
    gFeatureManager->mDispatcher->trigger(holder);

    igEndFrame();

    if (!alreadyRunningD3D11) {
        ID3D11Resource* resource = mBackBuffer11Tex.at(index).get();
        gDevice11on12->AcquireWrappedResources(&resource, 1);
    }

    auto thing = mBackBuffer11Rtv.at(index).get();
    gContext11->OMSetRenderTargets(1, &thing, NULL);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    D2D::endRender();

    if (!alreadyRunningD3D11) {
        ID3D11Resource* resource = mBackBuffer11Tex.at(index).get();
        gDevice11on12->ReleaseWrappedResources(&resource, 1);
        gContext11->Flush();
    }

    return oPresent(swapChain, syncInterval, flags);
}

HRESULT D3DHook::resizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height,
    DXGI_FORMAT newFormat, UINT swapChainFlags)
{
    if (Solstice::mUnloading.load())
        return oResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);

    releaseSwapChainTargets();

    return oResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

HRESULT D3DHook::resizeBuffers1(
    IDXGISwapChain3* swapChain,
    UINT bufferCount, UINT width, UINT height,
    DXGI_FORMAT format, UINT flags,
    const UINT* creationNodeMask,
    IUnknown* const* presentQueue)
{
    if (Solstice::mUnloading.load())
        return oResizeBuffers1(swapChain, bufferCount, width, height, format, flags, creationNodeMask, presentQueue);

    releaseSwapChainTargets();

    return oResizeBuffers1(
        swapChain, bufferCount, width, height, format, flags,
        creationNodeMask, presentQueue);
}

void D3DHook::initImGui(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    if (imGuiInitialized) return;

    auto* ci = ClientInstance::get();
    auto* guiData = ci ? ci->getGuiData() : nullptr;
    if (!guiData) return;

    ImGui::CreateContext();

    FontHelper::load();
    static bool onc = false;
    if (!onc)
    {
        if (ResourceLoader::Resources.contains("skinblinker.txt"))
        {
            auto res = &ResourceLoader::Resources["skinblinker.txt"];
            FileUtils::writeResourceToFile(res, FileUtils::getSolsticeDir() + "BlinkerSkins\\README.txt");
        }
        onc = true;
    }

    HWND targetWnd = wnd ? wnd : ProcUtils::getMinecraftWindow();
    ImGui_ImplWin32_Init(targetWnd);
    ImGui_ImplDX11_Init(device, deviceContext);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(guiData->mResolution.x, guiData->mResolution.y);

    imGuiInitialized = true;
}

void D3DHook::shutdownImGui()
{
    if (imGuiInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        imGuiInitialized = false;
    }
}

void D3DHook::igNewFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    auto* ci = ClientInstance::get();
    auto* guiData = ci ? ci->getGuiData() : nullptr;
    if (!guiData) return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(guiData->mResolution.x, guiData->mResolution.y);

    MathUtils::fov = RenderUtils::transform.mFov;
    MathUtils::displaySize = guiData->mResolution;
    MathUtils::origin = RenderUtils::transform.mOrigin;
}

void D3DHook::igEndFrame()
{
    ImGui::EndFrame();
    ImGui::Render();
}

void D3DHook::init()
{
    mName = "D3DHook";
    s_init();
}

void D3DHook::s_init()
{
    Solstice::console->info("Initializing D3DHook");
    if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success)
    {
        Solstice::console->info("Initialized kiero [D3D12]");
        void* _present = kiero::getMethod<&IDXGISwapChain::Present, void*>();
        void* _resize = kiero::getMethod<&IDXGISwapChain::ResizeBuffers, void*>();
        void* _resize1 = kiero::getMethod<&IDXGISwapChain3::ResizeBuffers1, void*>();
        void* _oPresent = nullptr;
        void* _oResize = nullptr;
        void* _oResize1 = nullptr;

        if (MH_CreateHook(_present, reinterpret_cast<void*>(&D3DHook::present), &_oPresent) != MH_OK) {
            Solstice::console->critical("Present hook fail");
            return;
        }
        oPresent = reinterpret_cast<decltype(oPresent)>(_oPresent);
        MH_EnableHook(_present);

        if (_resize && MH_CreateHook(_resize, reinterpret_cast<void*>(&D3DHook::resizeBuffers), &_oResize) == MH_OK) {
            oResizeBuffers = reinterpret_cast<decltype(oResizeBuffers)>(_oResize);
            MH_EnableHook(_resize);
        }
        if (_resize1 && MH_CreateHook(_resize1, reinterpret_cast<void*>(&D3DHook::resizeBuffers1), &_oResize1) == MH_OK) {
            oResizeBuffers1 = reinterpret_cast<decltype(oResizeBuffers1)>(_oResize1);
            MH_EnableHook(_resize1);
        }
        return;
    }
    else if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
    {
        Solstice::console->info("Initialized kiero [D3D11]");
        void* _present = kiero::getMethod<&IDXGISwapChain::Present, void*>();
        void* _resize = kiero::getMethod<&IDXGISwapChain::ResizeBuffers, void*>();
        void* _oPresent = nullptr;
        void* _oResize = nullptr;

        if (MH_CreateHook(_present, reinterpret_cast<void*>(&D3DHook::present), &_oPresent) != MH_OK) {
            Solstice::console->critical("Present hook fail");
            return;
        }
        if (MH_CreateHook(_resize, reinterpret_cast<void*>(&D3DHook::resizeBuffers), &_oResize) != MH_OK) {
            Solstice::console->critical("ResizeBuffers hook fail");
            MH_RemoveHook(_present);
            return;
        }

        oPresent = reinterpret_cast<decltype(oPresent)>(_oPresent);
        oResizeBuffers = reinterpret_cast<decltype(oResizeBuffers)>(_oResize);

        MH_EnableHook(_present);
        MH_EnableHook(_resize);
        return;
    }

    Solstice::console->error("Failed to initialize kiero");
    MessageBoxA(NULL, "Failed to initialize kiero", "Solstice", MB_OK | MB_ICONERROR);
}

void D3DHook::shutdown()
{
    s_shutdown();
}

void D3DHook::s_shutdown()
{
    Solstice::console->info("Shutting down D3DHook");
    FrameTransforms.reset();
    void* _present = kiero::getMethod<&IDXGISwapChain::Present, void*>();
    void* _resize1 = kiero::getMethod<&IDXGISwapChain3::ResizeBuffers1, void*>();
    void* _resize = kiero::getMethod<&IDXGISwapChain::ResizeBuffers, void*>();
    if (_resize) {
        MH_DisableHook(_resize);
        MH_RemoveHook(_resize);
    }
    if (_present) {
        MH_DisableHook(_present);
        MH_RemoveHook(_present);
    }
    if (_resize1) {
        MH_DisableHook(_resize1);
        MH_RemoveHook(_resize1);
    }
    kiero::shutdown();
}
