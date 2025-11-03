#pragma once
#include "raygui.h"
#include "raylib.h"
#include <string>
#include <vector>
#include "iDisposable.h"

namespace BreadEditor {
    // Anchor by parent (UI_X_Y)
    typedef enum UI_ANCHOR_TYPE
    {
        // default anchors
        UI_LEFT_TOP,
        UI_CENTER_TOP,
        UI_RIGHT_TOP,
        UI_LEFT_CENTER,
        UI_CENTER_CENTER,
        UI_RIGHT_CENTER,
        UI_LEFT_BOTTOM,
        UI_CENTER_BOTTOM,
        UI_RIGHT_BOTTOM,
        // fill types
        UI_FIT_ALL,
        UI_FIT_CENTER_VERTICAL,
        UI_FIT_CENTER_HORIZONTAL,
        UI_FIT_TOP_VERTICAL,
        UI_FIT_TOP_HORIZONTAL,
        UI_FIT_BOTTOM_VERTICAL,
        UI_FIT_BOTTOM_HORIZONTAL,
        UI_FIT_RIGHT_VERTICAL,
        UI_FIT_LEFT_VERTICAL,
    } UI_ANCHOR_TYPE;

    class UiElement : public BreadEngine::IDisposable
    {
    public:
        std::string id;

        virtual ~UiElement();

        virtual void draw(float deltaTime);

        virtual void update(float deltaTime);

        [[nodiscard]] Rectangle getBounds() const;

        [[nodiscard]] Vector2 getPosition() const;

        [[nodiscard]] Vector2 getSize() const;

        virtual void setState(GuiState nextState);

        void setPosition(const Vector2 &position);

        void setSize(const Vector2 &size);

        void setSizePercentOneTime(const Vector2 &percent);

        void setSizePercentPermanent(const Vector2 &percent);

        void setBounds(const Vector2 &position, const Vector2 &size);

        [[nodiscard]] float getSizeInPixByPercentOnlyX(const Vector2 &percent) const;

        [[nodiscard]] float getSizeInPixByPercentOnlyY(const Vector2 &percent) const;

        [[nodiscard]] Vector2 getSizeInPixByPercent(const Vector2 &percent) const;

        [[nodiscard]] const UiElement *getRootElement() const;

        [[nodiscard]] UiElement *getParentElement() const;

        [[nodiscard]] UiElement *const *getAllChilds() const;

        [[nodiscard]] UiElement *getChildById(const std::string &childId) const;

        void addChild(UiElement *child);

        void destroyChild(UiElement *child);

        void destroyChild(const std::string &childId);

        void setAnchor(UI_ANCHOR_TYPE newAnchor);

        void setPivot(const Vector2 &newPivot);

        void changeParent(UiElement *newParent);

        void setChildFirst(UiElement *child);
        void setChildLast(UiElement *child);

        void dispose() override;

    protected:
        GuiState state = STATE_NORMAL;

        Vector2 pivot{};
        Vector2 localPosition{0.0f, 0.0f};
        Vector2 localSize{1.0f, 1.0f};
        Vector2 sizeInPercents{-1, -1};
        UiElement *parent = nullptr;
        std::vector<UiElement *> childs{};
        Rectangle bounds{0, 0, 1, 1};
        Color bgColor = RAYWHITE;
        UI_ANCHOR_TYPE anchor = UI_LEFT_TOP;

        void computeBounds();

        UiElement &setup(const std::string &newId);

        UiElement &setup(const std::string &newId, UiElement *parentElement);

        [[nodiscard]] virtual Vector2 getAnchorPoint(const Rectangle &parentBounds) const;

        [[nodiscard]] virtual Vector2 getComputedSize(const Rectangle &effectiveParentBounds, const Vector2 &prelimPosition) const;

        virtual bool tryDeleteSelf();
    };
} // namespace BreadEditor
