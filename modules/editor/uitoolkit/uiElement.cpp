#include "uiElement.h"
#include <algorithm>

namespace BreadEditor {
    UiElement &UiElement::setup(const std::string &newId)
    {
        this->id = newId;
        return *this;
    }

    UiElement &UiElement::setup(const std::string &newId, UiElement *parentElement)
    {
        this->id = newId;
        if (parentElement)
        {
            this->parent = parentElement;
            parent->addChild(this);
        }

        return *this;
    }

    UiElement::~UiElement()
    {
        UiElement::dispose();
    }

    void UiElement::draw(const float deltaTime)
    {
        for (const auto child: childs)
        {
            child->draw(deltaTime);
        }
    }

    void UiElement::update(const float deltaTime)
    {
        computeBounds();
        for (const auto child: childs)
        {
            child->update(deltaTime);
        }
    }

    Rectangle UiElement::getBounds() const
    {
        return bounds;
    }

    Vector2 UiElement::getPosition() const
    {
        return localPosition;
    }

    Vector2 UiElement::getSize() const
    {
        return localSize;
    }

    void UiElement::setState(GuiState nextState)
    {
        state = nextState;
    }

    void UiElement::setPosition(const Vector2 &position)
    {
        localPosition = position;
    }

    void UiElement::setSize(const Vector2 &size)
    {
        localSize = size;
    }

    void UiElement::setSizePercentOneTime(const Vector2 &percent)
    {
        if (percent.x >= 0 && percent.y >= 0)
        {
            setSize(getSizeInPixByPercent(percent));
        }
        else if (percent.x >= 0 && percent.y < 0)
        {
            auto ySize = localSize.y;
            localSize.x = getSizeInPixByPercentOnlyX(percent);
            setSize({localSize.x, ySize});
        }
        else if (percent.x < 0 && percent.y >= 0)
        {
            auto xSize = localSize.x;
            localSize.y = getSizeInPixByPercentOnlyY(percent);
            setSize({xSize, localSize.y});
        }

        sizeInPercents = {-1, -1};
    }

    void UiElement::setSizePercentPermanent(const Vector2 &percent)
    {
        if (percent.x >= 0 && percent.y >= 0)
        {
            setSize(getSizeInPixByPercent(percent));
        }
        else if (percent.x >= 0 && percent.y < 0)
        {
            auto ySize = localSize.y;
            localSize.x = getSizeInPixByPercentOnlyX(percent);
            setSize({localSize.x, ySize});
        }
        else if (percent.x < 0 && percent.y >= 0)
        {
            auto xSize = localSize.x;
            localSize.y = getSizeInPixByPercentOnlyY(percent);
            setSize({xSize, localSize.y});
        }

        sizeInPercents = percent;
    }

    void UiElement::setBounds(const Vector2 &position, const Vector2 &size)
    {
        localPosition = position;
        localSize = size;
    }

    float UiElement::getSizeInPixByPercentOnlyX(const Vector2 &percent) const
    {
        return getSizeInPixByPercent(percent).x;
    }

    float UiElement::getSizeInPixByPercentOnlyY(const Vector2 &percent) const
    {
        return getSizeInPixByPercent(percent).y;
    }

    Vector2 UiElement::getSizeInPixByPercent(const Vector2 &percent) const
    {
        const Vector2 clampedPercent = {std::clamp(percent.x, 0.0f, 1.0f), std::clamp(percent.y, 0.0f, 1.0f)};
        Rectangle effectiveParentBounds;
        if (parent)
        {
            parent->computeBounds();
            effectiveParentBounds = parent->getBounds();
        }
        else
        {
            effectiveParentBounds = {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        }

        return {clampedPercent.x * effectiveParentBounds.width, clampedPercent.y * effectiveParentBounds.height};
    }

    const UiElement *UiElement::getRootElement() const
    {
        if (!parent)
        {
            return this;
        }

        return parent->getRootElement();
    }

    UiElement *UiElement::getParentElement() const
    {
        return this->parent;
    }

    UiElement *const *UiElement::getAllChilds() const
    {
        return childs.data();
    }

    UiElement *UiElement::getChildById(const std::string &childId) const
    {
        for (auto &component: childs)
        {
            if (component->id == childId)
            {
                return component;
            }
        }

        TraceLog(LOG_ERROR, "Could not find child with id '%s'", childId.c_str());
        return nullptr;
    }

    void UiElement::addChild(UiElement *child)
    {
        if (child && std::ranges::find(childs, child) == childs.end())
        {
            child->parent = this;
            childs.push_back(child);
        }
    }

    void UiElement::destroyChild(UiElement *child)
    {
        childs.erase(std::ranges::find(childs, child));
        child->tryDeleteSelf();
    }

    void UiElement::destroyChild(const std::string &childId)
    {
        for (const auto child: childs)
        {
            if (child->id == childId)
            {
                destroyChild(child);
            }
        }
    }

    void UiElement::setAnchor(const UI_ANCHOR_TYPE newAnchor)
    {
        anchor = newAnchor;
    }

    void UiElement::setPivot(const Vector2 &newPivot)
    {
        pivot = newPivot;
    }

    void UiElement::changeParent(UiElement *newParent)
    {
        if (parent)
        {
            parent->childs.erase(std::ranges::find(parent->childs, this));
            parent = nullptr;
        }

        parent = newParent;
        if (parent)
        {
            parent->childs.emplace_back(this);
        }
    }

    void UiElement::dispose()
    {
        if (isDisposed) return;

        isDisposed = true;
        for (const auto child: childs)
        {
            if (child == nullptr)
            {
                continue;
            }

            auto isDeleted = child->tryDeleteSelf();
            if (!isDeleted)
            {
                delete child;
            }
        }

        childs.clear();
        parent = nullptr;
    }

    void UiElement::computeBounds()
    {
        if (sizeInPercents.x >= 0 || sizeInPercents.y >= 0)
        {
            setSizePercentPermanent(sizeInPercents);
        }

        Rectangle effectiveParentBounds;
        if (parent)
        {
            effectiveParentBounds = parent->getBounds();
        }
        else
        {
            effectiveParentBounds = {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        }
        const Vector2 anchorPoint = getAnchorPoint(effectiveParentBounds);
        const Vector2 pivotOffset = {pivot.x * localSize.x, pivot.y * localSize.y};
        const Vector2 prelimPosition = {anchorPoint.x + localPosition.x - pivotOffset.x, anchorPoint.y + localPosition.y - pivotOffset.y};
        const Vector2 computedSize = getComputedSize(effectiveParentBounds, prelimPosition);
        bounds = {prelimPosition.x, prelimPosition.y, computedSize.x, computedSize.y};
    }

    Vector2 UiElement::getAnchorPoint(const Rectangle &effectiveParentBounds) const
    {
        switch (anchor)
        {
            case UI_LEFT_TOP: return {effectiveParentBounds.x, effectiveParentBounds.y};
            case UI_CENTER_TOP: return {effectiveParentBounds.x + effectiveParentBounds.width / 2.0f, effectiveParentBounds.y};
            case UI_RIGHT_TOP: return {effectiveParentBounds.x + effectiveParentBounds.width, effectiveParentBounds.y};
            case UI_LEFT_CENTER: return {effectiveParentBounds.x, effectiveParentBounds.y + effectiveParentBounds.height / 2.0f};
            case UI_CENTER_CENTER: return {effectiveParentBounds.x + effectiveParentBounds.width / 2.0f, effectiveParentBounds.y + effectiveParentBounds.height / 2.0f};
            case UI_RIGHT_CENTER: return {effectiveParentBounds.x + effectiveParentBounds.width, effectiveParentBounds.y + effectiveParentBounds.height / 2.0f};
            case UI_LEFT_BOTTOM: return {effectiveParentBounds.x, effectiveParentBounds.y + effectiveParentBounds.height};
            case UI_CENTER_BOTTOM: return {effectiveParentBounds.x + effectiveParentBounds.width / 2.0f, effectiveParentBounds.y + effectiveParentBounds.height};
            case UI_RIGHT_BOTTOM: return {effectiveParentBounds.x + effectiveParentBounds.width, effectiveParentBounds.y + effectiveParentBounds.height};

            case UI_FIT_ALL: return {effectiveParentBounds.x, effectiveParentBounds.y};
            case UI_FIT_CENTER_VERTICAL: return {effectiveParentBounds.x + effectiveParentBounds.width / 2.0f, effectiveParentBounds.y};
            case UI_FIT_CENTER_HORIZONTAL: return {effectiveParentBounds.x, effectiveParentBounds.y + effectiveParentBounds.height / 2.0f};
            case UI_FIT_TOP_VERTICAL: return {effectiveParentBounds.x, effectiveParentBounds.y};
            case UI_FIT_TOP_HORIZONTAL: return {effectiveParentBounds.x, effectiveParentBounds.y};
            case UI_FIT_BOTTOM_VERTICAL: return {effectiveParentBounds.x, effectiveParentBounds.y + effectiveParentBounds.height};
            case UI_FIT_BOTTOM_HORIZONTAL: return {effectiveParentBounds.x + effectiveParentBounds.width, effectiveParentBounds.y + effectiveParentBounds.height};
            case UI_FIT_RIGHT_VERTICAL: return {effectiveParentBounds.x + effectiveParentBounds.width, effectiveParentBounds.y};
            case UI_FIT_LEFT_VERTICAL: return {effectiveParentBounds.x, effectiveParentBounds.y};
            default: return {effectiveParentBounds.x, effectiveParentBounds.y};
        }
    }

    Vector2 UiElement::getComputedSize(const Rectangle &effectiveParentBounds, const Vector2 &prelimPosition) const
    {
        Vector2 size = localSize;
        bool fitX = false;
        bool fitY = false;
        switch (anchor)
        {
            case UI_FIT_ALL: fitX = true;
                fitY = true;
                break;
            case UI_FIT_CENTER_VERTICAL:
            case UI_FIT_TOP_VERTICAL:
            case UI_FIT_BOTTOM_VERTICAL:
            case UI_FIT_RIGHT_VERTICAL:
            case UI_FIT_LEFT_VERTICAL: fitY = true;
                break;
            case UI_FIT_CENTER_HORIZONTAL:
            case UI_FIT_TOP_HORIZONTAL:
            case UI_FIT_BOTTOM_HORIZONTAL: fitX = true;
                break;
            default: break;
        }
        if (fitX)
        {
            size.x = std::max(0.0f, effectiveParentBounds.x + effectiveParentBounds.width - prelimPosition.x);
        }
        if (fitY)
        {
            size.y = std::max(0.0f, effectiveParentBounds.y + effectiveParentBounds.height - prelimPosition.y);
        }
        return size;
    }

    bool UiElement::tryDeleteSelf()
    {
        return false;
    }
} // namespace BreadEditor
