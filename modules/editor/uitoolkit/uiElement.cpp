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
            this->_parent = parentElement;
            _parent->addChild(this);
        }

        return *this;
    }

    UiElement::~UiElement()
    {
        UiElement::dispose();
    }

    void UiElement::draw(const float deltaTime)
    {
        if (isDebugRectVisible)
        {
            drawDebugRect();
        }

        for (const auto child: _childs)
        {
            child->draw(deltaTime);
        }
    }

    void UiElement::update(const float deltaTime)
    {
        computeBounds();
        for (const auto child: _childs)
        {
            child->update(deltaTime);
        }
    }

    Rectangle UiElement::getBounds() const
    {
        return _bounds;
    }

    Vector2 UiElement::getPosition() const
    {
        return _localPosition;
    }

    Vector2 UiElement::getSize() const
    {
        if (_sizeInPercents.x >= 0 && _sizeInPercents.y >= 0)
        {
            return getSizeInPixByPercent(_sizeInPercents);
        }

        if (_sizeInPercents.x >= 0 && _sizeInPercents.y < 0)
        {
            const auto ySize = _localSize.y;
            const auto xSize = getSizeInPixByPercentOnlyX(_sizeInPercents);
            return {xSize, ySize};
        }

        if (_sizeInPercents.x < 0 && _sizeInPercents.y >= 0)
        {
            const auto xSize = _localSize.x;
            const auto ySize = getSizeInPixByPercentOnlyY(_sizeInPercents);
            return {xSize, ySize};
        }

        return _localSize;
    }

    void UiElement::setState(GuiState nextState)
    {
        _state = nextState;
    }

    void UiElement::setPosition(const Vector2 &position)
    {
        _localPosition = position;
    }

    void UiElement::setSize(const Vector2 &size)
    {
        _localSize = size;
    }

    void UiElement::setSizePercentOneTime(const Vector2 &percent)
    {
        if (percent.x >= 0 && percent.y >= 0)
        {
            setSize(getSizeInPixByPercent(percent));
        }
        else if (percent.x >= 0 && percent.y < 0)
        {
            const auto ySize = _localSize.y;
            _localSize.x = getSizeInPixByPercentOnlyX(percent);
            setSize({_localSize.x, ySize});
        }
        else if (percent.x < 0 && percent.y >= 0)
        {
            const auto xSize = _localSize.x;
            _localSize.y = getSizeInPixByPercentOnlyY(percent);
            setSize({xSize, _localSize.y});
        }

        _sizeInPercents = {-1, -1};
    }

    void UiElement::setSizePercentPermanent(const Vector2 &percent)
    {
        if (percent.x >= 0 && percent.y >= 0)
        {
            setSize(getSizeInPixByPercent(percent));
        }
        else if (percent.x >= 0 && percent.y < 0)
        {
            const auto ySize = _localSize.y;
            _localSize.x = getSizeInPixByPercentOnlyX(percent);
            setSize({_localSize.x, ySize});
        }
        else if (percent.x < 0 && percent.y >= 0)
        {
            const auto xSize = _localSize.x;
            _localSize.y = getSizeInPixByPercentOnlyY(percent);
            setSize({xSize, _localSize.y});
        }

        _sizeInPercents = percent;
    }

    void UiElement::setBounds(const Vector2 &position, const Vector2 &size)
    {
        _localPosition = position;
        _localSize = size;
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
        if (_parent)
        {
            _parent->computeBounds();
            effectiveParentBounds = _parent->getBounds();
        }
        else
        {
            effectiveParentBounds = {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        }

        return {clampedPercent.x * effectiveParentBounds.width, clampedPercent.y * effectiveParentBounds.height};
    }

    const UiElement *UiElement::getRootElement() const
    {
        if (!_parent)
        {
            return this;
        }

        return _parent->getRootElement();
    }

    UiElement *UiElement::getParentElement() const
    {
        return this->_parent;
    }

    UiElement *const *UiElement::getAllChilds() const
    {
        return _childs.data();
    }

    UiElement *UiElement::getChildById(const std::string &childId) const
    {
        for (auto &component: _childs)
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
        if (child && std::ranges::find(_childs, child) == _childs.end())
        {
            child->_parent = this;
            _childs.push_back(child);
        }
    }

    void UiElement::destroyChild(UiElement *child)
    {
        _childs.erase(std::ranges::find(_childs, child));
        auto isDeleted = child->tryDeleteSelf();
        if (!isDeleted)
        {
            delete child;
        }
    }

    void UiElement::destroyChild(const std::string &childId)
    {
        for (const auto child: _childs)
        {
            if (child->id == childId)
            {
                destroyChild(child);
            }
        }
    }

    void UiElement::setAnchor(const UI_ANCHOR_TYPE newAnchor)
    {
        _anchor = newAnchor;
    }

    void UiElement::setPivot(const Vector2 &newPivot)
    {
        _pivot = newPivot;
    }

    void UiElement::changeParent(UiElement *newParent)
    {
        if (_parent)
        {
            _parent->_childs.erase(std::ranges::find(_parent->_childs, this));
            _parent = nullptr;
        }

        _parent = newParent;
        if (_parent)
        {
            _parent->_childs.emplace_back(this);
        }
    }

    void UiElement::setChildFirst(UiElement *child)
    {
        if (const auto it = std::ranges::find(_childs, child); it != _childs.end())
        {
            _childs.erase(it);
            _childs.insert(_childs.begin(), child);
            return;
        }

        TraceLog(LOG_ERROR, "UiElement: Element %s not a child of %s", child->id.c_str(), id.c_str());
    }

    void UiElement::setChildLast(UiElement *child)
    {
        if (const auto it = std::ranges::find(_childs, child); it != _childs.end())
        {
            _childs.erase(it);
            addChild(child);
            return;
        }

        TraceLog(LOG_ERROR, "UiElement: Element %s not a child of %s", child->id.c_str(), id.c_str());
    }

    void UiElement::dispose()
    {
        if (_isDisposed) return;

        _isDisposed = true;
        for (const auto child: _childs)
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

        _childs.clear();
        _parent = nullptr;
    }

    void UiElement::drawDebugRect() const
    {
        GuiDummyRec(_bounds, "Debug");
    }

    void UiElement::computeBounds()
    {
        if (_sizeInPercents.x >= 0 || _sizeInPercents.y >= 0)
        {
            setSizePercentPermanent(_sizeInPercents);
        }

        Rectangle effectiveParentBounds;
        if (_parent)
        {
            effectiveParentBounds = _parent->getBounds();
        }
        else
        {
            effectiveParentBounds = {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        }
        const Vector2 anchorPoint = getAnchorPoint(effectiveParentBounds);
        const Vector2 pivotOffset = {_pivot.x * _localSize.x, _pivot.y * _localSize.y};
        const Vector2 prelimPosition = {anchorPoint.x + _localPosition.x - pivotOffset.x, anchorPoint.y + _localPosition.y - pivotOffset.y};
        const Vector2 computedSize = getComputedSize(effectiveParentBounds, prelimPosition);
        _bounds = {prelimPosition.x, prelimPosition.y, computedSize.x, computedSize.y};
    }

    Vector2 UiElement::getAnchorPoint(const Rectangle &effectiveParentBounds) const
    {
        switch (_anchor)
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
        Vector2 size = _localSize;
        bool fitX = false;
        bool fitY = false;
        switch (_anchor)
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
