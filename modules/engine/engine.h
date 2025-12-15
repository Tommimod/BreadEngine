#pragma once

#include "../../lib/engine/raylib.h"
#include "moduleLoader.h"
#include <string>
#include "node.h"
#include "objectPool.h"

namespace BreadEngine {
    class Engine
    {
    public:
        static ObjectPool<Node> nodePool;

        static Engine &getInstance();

        static Node &getRootNode();

        static float getDeltaTime();

        bool initialize(int width, int height, const char *title);

        void shutdown();

        static bool shouldClose();

        void beginFrame() const;

        void endFrame();

        void setupDefaultCamera();

        Camera3D &getCamera()
        {
            return _camera;
        }

        void loadGameModule(const char *path);

        void unloadGameModule();

        void callGameRender2D() const;

        void callGameRender3D() const;

        static std::string getAssetPath(const std::string &relativePath, const std::string &module = "");

        static bool isFileExists(const std::string &path);

        // Check if point is inside rectangle
        static bool isCollisionPointRec(Vector2 point, Rectangle rec);

        // Check if point is inside rectangle with hole
        static bool isCollisionPointRec(Vector2 point, Rectangle rec, Rectangle subtraction);

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

    namespace Input {
        bool isKeyPressed(int key);

        bool isKeyDown(int key);

        bool isMouseButtonPressed(int button);

        Vector2 getMousePosition();
    } // namespace Input

    namespace Rendering {
        void drawGrid(int slices, float spacing);

        void drawFPS(int posX, int posY);

        void clearBackground(Color color);
    } // namespace Rendering

    namespace Time {
        float getTime();

        float getFrameTime();

        int getFPS();

        void setTargetFPS(int fps);
    } // namespace Time
} // namespace BreadEngine
