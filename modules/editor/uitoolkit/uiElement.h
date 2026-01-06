#pragma once
#include "raygui.h"
#include "raylib.h"
#include <string>
#include <vector>
#include "iDisposable.h"

namespace BreadEditor {
    typedef enum LAYOUT_TYPE
    {
        LAYOUT_NONE,
        LAYOUT_HORIZONTAL,
        LAYOUT_VERTICAL
    } LAYOUT_TYPE;

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
        bool isActive = true;
        bool isStatic = false;
        bool isDebugRectVisible = false;

        virtual ~UiElement();

        [[nodiscard]] Rectangle &getBounds();

        [[nodiscard]] Vector2 &getPosition();

        [[nodiscard]] Vector2 getSize() const;

        virtual void draw(float deltaTime);

        virtual void update(float deltaTime);

        virtual void setState(GuiState nextState);

        void setPosition(const Vector2 &position);

        void setSize(const Vector2 &size);

        void setSizePercentOneTime(const Vector2 &percent);

        void setSizePercentPermanent(const Vector2 &percent);

        void setSizeMax(const Vector2 &maxSize);

        void setBounds(const Vector2 &position, const Vector2 &size);

        [[nodiscard]] Vector2 &getPivot();

        [[nodiscard]] float getSizeInPixByPercentOnlyX(const Vector2 &percent) const;

        [[nodiscard]] float getSizeInPixByPercentOnlyY(const Vector2 &percent) const;

        [[nodiscard]] Vector2 getSizeInPixByPercent(const Vector2 &percent) const;

        [[nodiscard]] const UiElement *getRootElement() const;

        [[nodiscard]] UiElement *getParentElement() const;

        [[nodiscard]] std::vector<UiElement *> &getAllChilds();

        [[nodiscard]] UiElement *getChildById(const std::string &childId) const;

        [[nodiscard]] int getChildCount() const;

        [[nodiscard]] UI_ANCHOR_TYPE getAnchor() const;

        [[nodiscard]] LAYOUT_TYPE getLayoutType() const;

        [[nodiscard]] UiElement *getNextSibling() const;

        [[nodiscard]] UiElement *getPrevSibling() const;

        [[nodiscard]] std::vector<UiElement *> getNextSiblingsByEqualHorizontal() const;

        [[nodiscard]] std::vector<UiElement *> getPrevSiblingsByEqualHorizontal() const;

        [[nodiscard]] std::vector<UiElement *> getNextSiblingsByEqualVertical() const;

        [[nodiscard]] std::vector<UiElement *> getPrevSiblingsByEqualVertical() const;

        [[nodiscard]] int getIndex() const;

        void setLayoutType(LAYOUT_TYPE layout);

        virtual void addChild(UiElement *child);

        virtual void destroyChild(UiElement *child);

        void destroyChild(const std::string &childId);

        void destroyAllChilds();

        void setAnchor(UI_ANCHOR_TYPE newAnchor);

        void setPivot(const Vector2 &newPivot);

        void changeParent(UiElement *newParent);

        void setChildFirst(UiElement *child);

        void setChildLast(UiElement *child);

        void dispose() override;

        void drawDebugRect() const;

        void computeBounds();

    protected:
        GuiState _state = STATE_NORMAL;
        Vector2 _pivot{};
        Vector2 _localPosition{0.0f, 0.0f};
        Vector2 _localSize{1.0f, 1.0f};
        Vector2 _sizeInPercents{-1, -1};
        Vector2 _maxSize{0, 0};
        UiElement *_parent = nullptr;
        std::vector<UiElement *> _childs{};
        Rectangle _bounds{0, 0, 1, 1};
        Color _bgColor = RAYWHITE;
        UI_ANCHOR_TYPE _anchor = UI_LEFT_TOP;
        LAYOUT_TYPE _layoutType = LAYOUT_NONE;

        UiElement &setup(const std::string &newId);

        UiElement &setup(const std::string &newId, UiElement *parentElement);

        [[nodiscard]] std::vector<UiElement *> getChildsSorterByHorizontal(bool reverse) const;

        [[nodiscard]] std::vector<UiElement *> getChildsSorterByVertical(bool reverse) const;

        [[nodiscard]] virtual Vector2 getAnchorPoint(const Rectangle &parentBounds) const;

        [[nodiscard]] virtual Vector2 getComputedSize(const Rectangle &effectiveParentBounds, const Vector2 &prelimPosition) const;

        virtual bool tryDeleteSelf();

    private:
        friend class Editor;

        void drawInternal(float deltaTime);

        void updateInternal(float deltaTime);
    };
} // namespace BreadEditor
