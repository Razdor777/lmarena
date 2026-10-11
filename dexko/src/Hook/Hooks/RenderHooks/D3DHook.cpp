//
//

#include "D3DHook.hpp"

#include <kiero.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/Rendering/bgfx_context.hpp>
#include <SDK/Minecraft/Rendering/GuiData.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/ProcUtils.hpp>
#include <Utils/MiscUtils/D2D.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <winrt/base.h>

// ImGui/D3D11 presenter hook (Kiero + ImGui backends).

using winrt::com_ptr;

typedef HRESULT(__thiscall* presentD3D)(IDXGISwapChain3*, UINT, UINT);
presentD3D oPresent;
typedef HRESULT(__thiscall* resizeBuffers)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
resizeBuffers oResizeBuffers;

static com_ptr<ID3D11Device> gDevice11;
static com_ptr<ID3D11DeviceContext> gContext11;
static com_ptr<ID3D12Device> gDevice12;
static std::vector<com_ptr<ID3D11Texture2D>> mBackBuffer11Tex = std::vector<com_ptr<ID3D11Texture2D>>();
static std::vector<com_ptr<ID3D11RenderTargetView>> mBackBuffer11Rtv =
        std::vector<com_ptr<ID3D11RenderTargetView>>();

static com_ptr<ID3D11DeviceContext> gDevice_context11;
static com_ptr<ID3D11On12Device> gDevice11on12;

static bool imGuiInitialized = false;

static HWND wnd = NULL;
static WNDPROC oWndProc;

static IDXGISwapChain3* gSwapChain = nullptr;

#define CHECK_HR(hr) if (FAILED(hr)) { spdlog::error("HRESULT failed: {0}", hr); return hr; }

static bool alreadyRunningD3D11 = false;
static bool d3dInitImGui = false;

#define BUFFER_COUNT 3



HRESULT D3DHook::present(IDXGISwapChain3* swapChain, UINT syncInterval, UINT flags)
{
    // Ejecting (or already ejected): never run another line of our code on the
    // render thread, otherwise the module can be unmapped while this thread is
    // still inside it.
    if (Dexko::mRequestEject || Dexko::mUnloading.load())
    {
        return oPresent(swapChain, syncInterval, flags);
    }
    gSwapChain = swapChain;
    static bool once = false;
    if (!once)
    {
        if (swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void **>(&gDevice11)) == S_OK) {
            spdlog::info("[D3D] D3D11 Device acquired");
            alreadyRunningD3D11 = true;
            static bool msgwarn = false;
            if (!msgwarn)
            {
                msgwarn = true;
            }
        }

        if (swapChain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void **>(&gDevice12)) == S_OK) {
            spdlog::info("[D3D] D3D12 Device acquired");

            if (Dexko::Prefs->mFallbackToD3D11)
            {
                ID3D12Device* bad_device;
                if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&bad_device))))
                {
                    spdlog::warn("Removing D3D12 device [preferred fallback]");
                    reinterpret_cast<ID3D12Device5*>(bad_device)->RemoveDevice();
                    return oPresent(swapChain, syncInterval, flags);
                }

            }

            UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;

            ID3D12CommandQueue* pacq = reinterpret_cast<bgfx_d3d12_RendererContextD3D12*>(bgfx_context::get()->getRenderContext())->getCommandQueue();
            ID3D12CommandQueue* realQueue = {};

            CHECK_HR(pacq->QueryInterface(IID_PPV_ARGS(&realQueue)));

            CHECK_HR(D3D11On12CreateDevice(
                    gDevice12.get(), // the D3D12 device
                    deviceFlags, // flags for the D3D11 device
                    nullptr, // array of feature levels (null is default, which is what the game uses i assume)
                    0, // number of feature levels in that array
                    reinterpret_cast<IUnknown **>(&pacq), // array of command queues (we only have one)
                    1, // number of command queues in that array
                    0, // node mask (0 is a magic number i actually don't know what it does)
                    gDevice11.put(), // the D3D11 device we get back
                    gDevice_context11.put(), // the D3D11 device context we get back
                    nullptr // the feature level we get back (we don't care)
            ));
        }

        once = true;
    }

    if (forceFallback && !alreadyRunningD3D11)
    {
        spdlog::warn("Forcing fallback to D3D11");

        // Same rule as the resize path: the back buffers are about to be
        // recreated, so the "already built" flag has to go with them. It used to
        // be left true, and the very next frame indexed the just-cleared
        // vectors (mBackBuffer11Tex.at(index)) -> std::out_of_range.
        mBackBuffer11Rtv.clear();
        mBackBuffer11Tex.clear();
        gContext11->Flush();

        gDevice11on12->Release();
        gDevice11on12 = nullptr;
        d3dInitImGui = false;

        ID3D12Device* bad_device;
        if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&bad_device))))
        {
            spdlog::warn("Removing D3D12 device [user requested fallback]");
            reinterpret_cast<ID3D12Device5*>(bad_device)->RemoveDevice();
        } else {
            spdlog::error("Failed to get D3D12 device");
        }

        forceFallback = false;
        once = false;

        return oPresent(swapChain, syncInterval, flags);
    }

    if (FrameTransforms)
        while(FrameTransforms->size() > transformDelay)
        {
            RenderUtils::transform = FrameTransforms->front();
            FrameTransforms->pop();
        }
    else
    {
        spdlog::error("FrameTransforms is null");
    }

    int count = alreadyRunningD3D11 ? 1 : BUFFER_COUNT;

    if (!d3dInitImGui) {
        mBackBuffer11Tex.resize(count);
        mBackBuffer11Rtv.resize(count);
        if (alreadyRunningD3D11) {
            gDevice11->GetImmediateContext(gContext11.put());
            DXGI_SWAP_CHAIN_DESC sd;
            swapChain->GetDesc(&sd);
            swapChain->GetBuffer(0, IID_PPV_ARGS(mBackBuffer11Tex.at(0).put()));
            gDevice11->CreateRenderTargetView(mBackBuffer11Tex.at(0).get(), NULL, mBackBuffer11Rtv.at(0).put());
        } else {
            gDevice11->GetImmediateContext(gContext11.put());
            DXGI_SWAP_CHAIN_DESC sd;
            CHECK_HR(swapChain->GetDesc(&sd));
            wnd = sd.OutputWindow;

            D3D11_RESOURCE_FLAGS backBuffer11Flags = {};
            backBuffer11Flags.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            for (int i = 0; i < BUFFER_COUNT; i++) {

                CHECK_HR(gDevice11->QueryInterface(IID_PPV_ARGS(&gDevice11on12)));
                winrt::com_ptr<ID3D12Resource> buffer = nullptr;

                CHECK_HR(swapChain->GetBuffer(i, IID_PPV_ARGS(buffer.put())));

                CHECK_HR(gDevice11on12->CreateWrappedResource(buffer.get(), &backBuffer11Flags,
                                                        D3D12_RESOURCE_STATE_PRESENT,
                                                        D3D12_RESOURCE_STATE_PRESENT,
                                                        IID_PPV_ARGS(mBackBuffer11Tex.at(i).put())));

                D3D11_TEXTURE2D_DESC texDesc;
                mBackBuffer11Tex.at(i)->GetDesc(&texDesc);
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Format = texDesc.Format;
                rtvDesc.Texture2D.MipSlice = 0;

                CHECK_HR(gDevice11->CreateRenderTargetView(mBackBuffer11Tex.at(i).get(), nullptr,
                                                     mBackBuffer11Rtv.at(i).put()));
            }
            oWndProc = (WNDPROC)SetWindowLongPtr(wnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
        d3dInitImGui = true;
    }

    float dpi = GetDpiForWindow(wnd);
    UINT index = reinterpret_cast<IDXGISwapChain3 *>(swapChain)->GetCurrentBackBufferIndex();

    index = alreadyRunningD3D11 ? 0 : index;


    initImGui(gDevice11.get(), gContext11.get());

    // No ImGui this frame (GuiData is not there yet, or the window is mid
    // resize). Hand the frame straight back to the game instead of touching an
    // uninitialised context — this is the path that crashed on F11 and on
    // window resizing.
    if (!imGuiInitialized)
        return oPresent(swapChain, syncInterval, flags);

    D2D::init(swapChain, gDevice11.get());

    static ImVec2 lastWindowSize = ImGui::GetIO().DisplaySize;

    auto* ci = ClientInstance::get();
    auto* guiData = ci ? ci->getGuiData() : nullptr;
    ImVec2 windowSize = guiData
        ? ImVec2(guiData->mResolution.x, guiData->mResolution.y)
        : lastWindowSize;

    lastWindowSize = windowSize;

    winrt::com_ptr<IDXGISurface> surface;
    mBackBuffer11Tex.at(index).try_as(surface);
    D2D::beginRender(surface.get(), dpi);

    igNewFrame();

    auto holder = nes::make_holder<RenderEvent>();
    gFeatureManager->mDispatcher->trigger(holder);

    igEndFrame();

    if (!alreadyRunningD3D11) {
        ID3D11Resource *resource = mBackBuffer11Tex.at(index).get();
        gDevice11on12->AcquireWrappedResources(&resource, 1);
    }


    auto thing = mBackBuffer11Rtv.at(index).get();
    gContext11->OMSetRenderTargets(1, &thing, NULL);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    D2D::endRender();

    if (!alreadyRunningD3D11) {
        ID3D11Resource *resource = mBackBuffer11Tex.at(index).get();
        gDevice11on12->ReleaseWrappedResources(&resource, 1);

        gContext11->Flush();
    }


    return oPresent(swapChain, syncInterval, flags);
}

HRESULT D3DHook::resizeBuffers(IDXGISwapChain3* swapChain, UINT bufferCount, UINT width, UINT height,
    DXGI_FORMAT newFormat, UINT swapChainFlags)
{
    // Ejecting: never touch our state from the render thread again.
    if (Dexko::mUnloading.load())
        return oResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);

    if (d3dInitImGui)
    {
        // EVERY reference to the back buffer has to be gone before ResizeBuffers,
        // otherwise DXGI fails the call and every later GetBuffer() hands back a
        // buffer that no longer matches the swap chain (visual corruption, then
        // crashes). The one that was always missed was the render target still
        // bound on the immediate context.
        if (gContext11)
        {
            ID3D11RenderTargetView* nullTargets[1] = { nullptr };
            gContext11->OMSetRenderTargets(1, nullTargets, nullptr);
            gContext11->Flush();
        }

        D2D::shutdown();
        shutdownImGui();

        mBackBuffer11Rtv.clear();
        mBackBuffer11Tex.clear();

        d3dInitImGui = false;
    }

    return oResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

void D3DHook::initImGui(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    if (imGuiInitialized) return;

    // GuiData vanishes for a moment on every swap chain resize and while a world
    // is loading. Bailing out leaves imGuiInitialized false, and present() then
    // skips the whole frame instead of dereferencing a null resolution.
    auto* ci = ClientInstance::get();
    auto* guiData = ci ? ci->getGuiData() : nullptr;
    if (!guiData) return;

    ImGui::CreateContext();

    FontHelper::load();

    ImGui_ImplWin32_Init(ProcUtils::getMinecraftWindow());
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

    // Keep the previous display size when GuiData is momentarily gone; a stale
    // size for one frame is harmless, a null dereference is not.
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
};

void D3DHook::init()
{
    mName = "D3DHook";
    s_init();
}

void D3DHook::s_init()
{
    Dexko::console->info("Initializing D3DHook");
    // Attempt to init on D3D12
    if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success)
    {
        Dexko::console->info("Initialized kiero [D3D12]");
        kiero::bind(140, reinterpret_cast<void**>(&oPresent), reinterpret_cast<void*>(present));
        kiero::bind(145, reinterpret_cast<void**>(&oResizeBuffers), reinterpret_cast<void*>(resizeBuffers));
        return;
        // Else, attempt to init on D3D11
    } else if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
    {
        Dexko::console->info("Initialized kiero [D3D11]");
        kiero::bind(8, reinterpret_cast<void**>(&oPresent), reinterpret_cast<void*>(present));
        kiero::bind(13, reinterpret_cast<void**>(&oResizeBuffers), reinterpret_cast<void*>(resizeBuffers));
        return;
    }
    Dexko::console->error("Failed to initialize kiero");
    MessageBoxA(NULL, "Failed to initialize kiero", "Dexko", MB_OK | MB_ICONERROR);
}

void D3DHook::shutdown()
{
    s_shutdown();
}

void D3DHook::s_shutdown()
{
    Dexko::console->info("Shutting down D3DHook");
    FrameTransforms.reset();
    kiero::unbind(8);
    kiero::unbind(13);
    kiero::unbind(140);
    kiero::unbind(145);
    kiero::shutdown();

    //if (imGuiInitialized) shutdownImGui();

    mBackBuffer11Rtv.clear();
    mBackBuffer11Tex.clear();
    gContext11->Flush();

    alreadyRunningD3D11 = false;
    d3dInitImGui = false;
    imGuiInitialized = false;


}
