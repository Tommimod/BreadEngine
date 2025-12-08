#pragma once

#include "../../lib/engine/raylib.h"
#include "moduleLoader.h"
#include <string>
#include "node.h"
#include "objectPool.h"

namespace BreadEngine
{
    class Engine
    {
    public:
        static Engine &GetInstance();

        static ObjectPool<Node> nodePool;

        static Node& getRootNode();

        static float GetDeltaTime();

        bool Initialize(int width, int height, const char *title);

        void Shutdown();

        static bool ShouldClose();

        void BeginFrame() const;

        void EndFrame();

        void SetupDefaultCamera();

        Camera3D &GetCamera()
        {
            return _camera;
        }

        void LoadGameModule(const char *path);

        void UnloadGameModule();

        void CallGameRender2D() const;

        void CallGameRender3D() const;

        static std::string GetAssetPath(const std::string &relativePath, const std::string &module = "");

        static bool FileExists(const std::string &path);

        // Check if point is inside rectangle
        static bool IsCollisionPointRec(Vector2 point, Rectangle rec);
        // Check if point is inside rectangle with hole
        static bool IsCollisionPointRec(Vector2 point, Rectangle rec, Rectangle subtraction);

    private:
        Engine();
        ~Engine() = default;
        static Node _rootNode;

        Camera3D _camera{};
        ModuleLoader *_gameModuleLoader = nullptr;

        typedef void (*GameInitFunc)();

        typedef void (*GameUpdateFunc)();

        typedef void (*GameRender2DFunc)();

        typedef void (*GameRender3DFunc)();

        typedef void (*GameShutdownFunc)();

        GameInitFunc _gameInit = nullptr;
        GameUpdateFunc _gameUpdate = nullptr;
        GameRender2DFunc _gameRender2D = nullptr;
        GameRender3DFunc _gameRender3D = nullptr;
        GameShutdownFunc _gameShutdown = nullptr;
    };

    namespace Input
    {
        bool IsKeyPressed(int key);

        bool IsKeyDown(int key);

        bool IsMouseButtonPressed(int button);

        Vector2 GetMousePosition();
    } // namespace Input

    namespace Rendering
    {
        void DrawGrid(int slices, float spacing);

        void DrawFPS(int posX, int posY);

        void ClearBackground(Color color);
    } // namespace Rendering

    namespace Time
    {
        float GetTime();

        float GetFrameTime();

        int GetFPS();

        void SetTargetFPS(int fps);
    } // namespace Time
} // namespace BreadEngine
