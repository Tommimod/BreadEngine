#include <raylib.h>

#include <EngineFactoryOpenGL.h>
#include <RefCntAutoPtr.hpp>

#include <cstdio>

using namespace Diligent;

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Diligent POC - GL context sharing (viewport interop)");

    void *hwnd = GetWindowHandle();
    printf("[spike] raylib created window + GL context, native handle = %p\n", hwnd);

    RefCntAutoPtr<IEngineFactoryOpenGL> pFactoryGL(GetEngineFactoryOpenGL());
    if (!pFactoryGL)
    {
        printf("[spike] FAILED: GetEngineFactoryOpenGL() returned null\n");
        return 1;
    }

    // Leave EngineCI.Window.hWnd unset (null): a non-null handle routes AttachToActiveGLContext's
    // Win32 implementation through the "create our own context" path, which calls SetPixelFormat
    // on the HDC - and Windows forbids setting a pixel format twice on the same HDC, so it fails
    // since raylib/GLFW already set one. Leaving it null takes the actual "attach" branch, which
    // just grabs wglGetCurrentContext() (the context raylib already made current) and does nothing
    // to the pixel format.
    EngineGLCreateInfo EngineCI;

    RefCntAutoPtr<IRenderDevice> pDevice;
    RefCntAutoPtr<IDeviceContext> pContext;
    pFactoryGL->AttachToActiveGLContext(EngineCI, &pDevice, &pContext);

    if (!pDevice || !pContext)
    {
        printf("[spike] FAILED: AttachToActiveGLContext did not produce a device/context on raylib's existing GL context\n");
        return 1;
    }
    printf("[spike] Diligent attached to raylib's already-active GL context OK\n");

    TextureDesc RTDesc;
    RTDesc.Name = "spike offscreen RT";
    RTDesc.Type = RESOURCE_DIM_TEX_2D;
    RTDesc.Width = 256;
    RTDesc.Height = 256;
    RTDesc.Format = TEX_FORMAT_RGBA8_UNORM;
    RTDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    RTDesc.MipLevels = 1;

    RefCntAutoPtr<ITexture> pRT;
    pDevice->CreateTexture(RTDesc, nullptr, &pRT);
    if (!pRT)
    {
        printf("[spike] FAILED: CreateTexture for the offscreen render target failed\n");
        return 1;
    }

    ITextureView *pRTV = pRT->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    const Uint64 glHandle = pRT->GetNativeHandle();
    printf("[spike] Diligent-owned GL texture native handle (GLuint) = %llu\n", (unsigned long long) glHandle);

    // Wrap the Diligent-owned GL texture name directly as a raylib Texture2D - zero copy.
    Texture raylibView{};
    raylibView.id = (unsigned int) glHandle;
    raylibView.width = 256;
    raylibView.height = 256;
    raylibView.mipmaps = 1;
    raylibView.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    int frames = 0;
    const int totalFrames = 30;
    while (!WindowShouldClose() && frames < totalFrames)
    {
        // Diligent renders into the offscreen RT (distinct orange so it's visually obvious in the screenshot)
        constexpr float clearColor[] = {0.95f, 0.55f, 0.1f, 1.0f};
        pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->ClearRenderTarget(pRTV, clearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // raylib's rlgl caches GL state (currently-bound framebuffer, viewport) on its own side and
        // has no idea Diligent just rebound the framebuffer to our offscreen RT above. Unbind Diligent's
        // render targets and invalidate its cache before handing control back, so raylib's next
        // BeginDrawing draws into the real backbuffer instead of continuing to draw into our RT.
        pContext->SetRenderTargets(0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        pContext->InvalidateState();
        pContext->Flush();

        // raylib draws its own frame AND blits the Diligent-rendered texture into it, same as
        // viewportWindow.cpp's existing R3D_SetResolution + DrawTexturePro pattern would.
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Diligent GL-backend texture blitted via raylib DrawTexturePro:", 10, 10, 16, BLACK);
        DrawTexturePro(raylibView,
                       {0, 0, (float) raylibView.width, (float) raylibView.height},
                       {50, 50, 300, 300},
                       {0, 0}, 0.0f, WHITE);
        EndDrawing();

        if (frames == totalFrames - 1)
        {
            TakeScreenshot("gl_interop_result.png");
        }
        frames++;
    }

    printf("[spike] Ran %d frames blitting a Diligent OpenGL-backend texture into a raylib-drawn frame via DrawTexturePro, no crash.\n", frames);

    pRTV = nullptr;
    pRT.Release();
    pContext.Release();
    pDevice.Release();
    CloseWindow();
    return 0;
}
