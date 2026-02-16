#pragma once
#include <string>
#include "inspectorObject.h"

namespace BreadEngine {
    class Node;

    struct Component : InspectorStruct
    {
        Component() = default;

        void setOwner(Node *parent);

        ~Component() override;

        virtual void init();

        virtual void start();

        virtual void update(float deltaTime);

        virtual void fixedUpdate(float fixedDeltaTime);

        virtual void onFrameStart(float deltaTime);

        virtual void onFrameEnd(float deltaTime);

        virtual void destroy();

        [[nodiscard]] virtual std::string toString() const;

        [[nodiscard]] Node *getParent() const;

        [[nodiscard]] bool getIsActive() const;

        virtual void setIsActive(bool nextActive);

    protected:
        Node *_parent = nullptr;
        bool _isActive = true;
    };
} // namespace BreadEngine
