#include <raylib.h>

#include <EngineFactoryVk.h>
#include <RefCntAutoPtr.hpp>

#include <cstdio>

using namespace Diligent;

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Diligent POC - raylib GL + Diligent Vulkan coexistence");

    void *hwnd = GetWindowHandle();
    printf("[spike] raylib created window, native handle = %p\n", hwnd);

    RefCntAutoPtr<IEngineFactoryVk> pFactoryVk(GetEngineFactoryVk());
    if (!pFactoryVk)
    {
        printf("[spike] FAILED: GetEngineFactoryVk() returned null\n");
        return 1;
    }

    EngineVkCreateInfo EngineCI;
    RefCntAutoPtr<IRenderDevice> pDevice;
    IDeviceContext *ppContexts[1] = {};
    pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &pDevice, ppContexts);
    RefCntAutoPtr<IDeviceContext> pContext(ppContexts[0]);

    if (!pDevice || !pContext)
    {
        printf("[spike] FAILED: CreateDeviceAndContextsVk did not produce a device/context\n");
        return 1;
    }
    printf("[spike] Vulkan device + immediate context created OK\n");

    Win32NativeWindow Window{hwnd};
    SwapChainDesc SCDesc;
    RefCntAutoPtr<ISwapChain> pSwapChain;
    pFactoryVk->CreateSwapChainVk(pDevice, pContext, SCDesc, Window, &pSwapChain);

    if (!pSwapChain)
    {
        printf("[spike] FAILED: CreateSwapChainVk could not create a swap chain on raylib's HWND\n");
        return 1;
    }
    printf("[spike] Vulkan swap chain created OK on the SAME HWND raylib owns\n");

    int frames = 0;
    while (!WindowShouldClose() && frames < 60)
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("raylib GL draw + Diligent Vulkan swapchain present, same HWND", 10, 10, 18, BLACK);
        DrawText(TextFormat("frame %d", frames), 10, 40, 18, DARKGRAY);
        EndDrawing();

        ITextureView *pRTV = pSwapChain->GetCurrentBackBufferRTV();
        constexpr float clearColor[] = {0.15f, 0.25f, 0.45f, 1.0f};
        pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearRenderTarget(pRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pSwapChain->Present();

        frames++;
    }

    printf("[spike] Ran %d frames with both raylib (GL) drawing and Diligent (Vulkan) presenting on the same HWND, no crash.\n", frames);

    pSwapChain.Release();
    pContext.Release();
    pDevice.Release();
    CloseWindow();
    return 0;
}
