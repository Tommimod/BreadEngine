#include "uiElement.h"
#include <algorithm>

#include "raymath.h"

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

    std::vector<UiElement *> UiElement::getChildsSorterByHorizontal(const bool reverse) const
    {
        std::vector<UiElement *> sortedChilds = _childs;
        if (!reverse)
        {
            std::ranges::sort(sortedChilds, [](UiElement *a, UiElement *b)
            {
                return a->getBounds().x <= b->getBounds().x;
            });
        }
        else
        {
            std::ranges::sort(sortedChilds, [](UiElement *a, UiElement *b)
            {
                return a->getBounds().x >= b->getBounds().x;
            });
        }
        return sortedChilds;
    }

    std::vector<UiElement *> UiElement::getChildsSorterByVertical(const bool reverse) const
    {
        std::vector<UiElement *> sortedChilds = _childs;
        if (!reverse)
        {
            std::ranges::sort(sortedChilds, [](UiElement *a, UiElement *b)
            {
                return a->getBounds().y <= b->getBounds().y;
            });
        }
        else
        {
            std::ranges::sort(sortedChilds, [](UiElement *a, UiElement *b)
            {
                return a->getBounds().y >= b->getBounds().y;
            });
        }
        return sortedChilds;
    }

    UiElement::~UiElement()
    {
        UiElement::dispose();
    }

    void UiElement::draw(const float deltaTime)
    {
        if (!isActive) return;

        GuiSetStyle(LABEL, TEXT_ALIGNMENT, 0);
        GuiSetStyle(DROPDOWNBOX, TEXT_ALIGNMENT, 1);
        GuiSetStyle(BUTTON, TEXT_ALIGNMENT, 0);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
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
        if (!isActive) return;

        computeBounds();
        for (const auto child: _childs)
        {
            child->update(deltaTime);
        }
    }

    Rectangle &UiElement::getBounds()
    {
        return _bounds;
    }

    Vector2 &UiElement::getPosition()
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
        if (isStatic) return;
        _localPosition = position;
    }

    void UiElement::setSize(const Vector2 &size)
    {
        if (isStatic) return;
        _localSize = size;
    }

    void UiElement::setSizePercentOneTime(const Vector2 &percent)
    {
        if (isStatic) return;
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
    }

    void UiElement::setSizePercentPermanent(const Vector2 &percent)
    {
        if (isStatic) return;
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

    void UiElement::setSizeMax(const Vector2 &maxSize)
    {
        _maxSize = maxSize;
    }

    void UiElement::setBounds(const Vector2 &position, const Vector2 &size)
    {
        if (isStatic) return;
        _localPosition = position;
        _localSize = size;
    }

    Vector2 &UiElement::getPivot()
    {
        return _pivot;
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

    std::vector<UiElement *> &UiElement::getAllChilds()
    {
        return _childs;
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

        return nullptr;
    }

    int UiElement::getChildCount() const
    {
        return static_cast<int>(_childs.size());
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
        if (const auto isDeleted = child->tryDeleteSelf(); !isDeleted)
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

    void UiElement::destroyAllChilds()
    {
        if (_childs.empty()) return;

        const auto childCount = getChildCount();
        for (auto i = childCount - 1; i >= 0; i--)
        {
            destroyChild(_childs[i]);
        }
    }

    void UiElement::setAnchor(const UI_ANCHOR_TYPE newAnchor)
    {
        if (isStatic) return;
        _anchor = newAnchor;
    }

    UI_ANCHOR_TYPE UiElement::getAnchor() const
    {
        return _anchor;
    }

    LAYOUT_TYPE UiElement::getLayoutType() const
    {
        return _layoutType;
    }

    UiElement *UiElement::getNextSibling() const
    {
        if (_parent == nullptr) return nullptr;
        const auto it = std::ranges::find(_parent->_childs, this);
        if (it == _parent->_childs.end() || it + 1 == _parent->_childs.end()) return nullptr;
        return *(it + 1);
    }

    UiElement *UiElement::getPrevSibling() const
    {
        if (_parent == nullptr) return nullptr;
        const auto it = std::ranges::find(_parent->_childs, this);
        if (it == _parent->_childs.begin() || it == _parent->_childs.end()) return nullptr;
        return *(it - 1);
    }

    std::vector<UiElement *> UiElement::getNextSiblingsByEqualHorizontal() const
    {
        std::vector<UiElement *> siblings{};
        if (_parent == nullptr) return siblings;
        const auto currentX = _bounds.x;
        const auto fromY = _bounds.y;
        const auto toY = _bounds.y + _bounds.height;
        bool isFoundOne = false;
        auto foundedX = 0.0f;
        for (const auto childs = _parent->getChildsSorterByHorizontal(false); auto child: childs)
        {
            if (child->isStatic) continue;
            const auto childBounds = child->getBounds();
            const auto childLastPoint = childBounds.y + childBounds.height;
            if ((childBounds.y < fromY && childLastPoint < fromY) || (childBounds.y > toY && childLastPoint > toY)) continue;

            if (const auto childX = childBounds.x; !isFoundOne && childX > currentX)
            {
                isFoundOne = true;
                foundedX = childX;
                siblings.emplace_back(child);
            }
            else if (isFoundOne && childX == foundedX)
            {
                siblings.emplace_back(child);
            }
        }

        return siblings;
    }

    std::vector<UiElement *> UiElement::getPrevSiblingsByEqualHorizontal() const
    {
        std::vector<UiElement *> siblings{};
        if (_parent == nullptr) return siblings;
        const auto currentX = _bounds.x;
        const auto fromY = _bounds.y;
        const auto toY = _bounds.y + _bounds.height;
        bool isFoundOne = false;
        auto foundedX = 0.0f;
        for (const auto childs = _parent->getChildsSorterByHorizontal(true); auto child: childs)
        {
            if (child->isStatic) continue;
            const auto childBounds = child->getBounds();
            const auto childLastPoint = childBounds.y + childBounds.height;
            if ((childBounds.y < fromY && childLastPoint < fromY) || (childBounds.y > toY && childLastPoint > toY)) continue;

            if (const auto childX = childBounds.x; !isFoundOne && childX < currentX)
            {
                isFoundOne = true;
                foundedX = childX;
                siblings.emplace_back(child);
            }
            else if (isFoundOne && childX == foundedX)
            {
                siblings.emplace_back(child);
            }
        }

        return siblings;
    }

    std::vector<UiElement *> UiElement::getNextSiblingsByEqualVertical() const
    {
        std::vector<UiElement *> siblings{};
        if (_parent == nullptr) return siblings;
        const auto currentY = _bounds.y;
        const auto fromX = _bounds.x;
        const auto toX = _bounds.x + _bounds.width;
        bool isFoundOne = false;
        auto foundedY = 0.0f;
        for (const auto childs = _parent->getChildsSorterByVertical(false); auto child: childs)
        {
            if (child->isStatic) continue;
            const auto childBounds = child->getBounds();
            const auto childLastPoint = childBounds.x + childBounds.width;
            if ((childBounds.x < fromX && childLastPoint < fromX) || (childBounds.x > toX && childLastPoint > toX)) continue;

            if (const auto childY = childBounds.y; !isFoundOne && childY > currentY)
            {
                isFoundOne = true;
                foundedY = childY;
                siblings.emplace_back(child);
            }
            else if (isFoundOne && childY == foundedY)
            {
                siblings.emplace_back(child);
            }
        }

        return siblings;
    }

    std::vector<UiElement *> UiElement::getPrevSiblingsByEqualVertical() const
    {
        std::vector<UiElement *> siblings{};
        if (_parent == nullptr) return siblings;
        const auto currentY = _bounds.y;
        const auto fromX = _bounds.x;
        const auto toX = _bounds.x + _bounds.width;
        bool isFoundOne = false;
        auto foundedY = 0.0f;
        for (const auto childs = _parent->getChildsSorterByVertical(true); auto child: childs)
        {
            if (child->isStatic) continue;
            const auto childBounds = child->getBounds();
            const auto childLastPoint = childBounds.x + childBounds.width;
            if ((childBounds.x < fromX && childLastPoint < fromX) || (childBounds.x > toX && childLastPoint > toX)) continue;

            if (const auto childY = childBounds.y; !isFoundOne && childY < currentY)
            {
                isFoundOne = true;
                foundedY = childY;
                siblings.emplace_back(child);
            }
            else if (isFoundOne && childY == foundedY)
            {
                siblings.emplace_back(child);
            }
        }

        return siblings;
    }

    int UiElement::getIndex() const
    {
        if (_parent == nullptr) return -1;
        return std::ranges::find(_parent->_childs, this) - _parent->_childs.begin();
    }

    void UiElement::setLayoutType(const LAYOUT_TYPE layout)
    {
        if (isStatic) return;
        _layoutType = layout;
    }

    void UiElement::setPivot(const Vector2 &newPivot)
    {
        if (isStatic) return;
        _pivot = newPivot;
    }

    void UiElement::changeParent(UiElement *newParent)
    {
        if (_parent != nullptr)
        {
            _parent->_childs.erase(std::ranges::find(_parent->_childs, this));
            _parent = nullptr;
        }

        _parent = newParent;
        if (_parent != nullptr)
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
            _childs.push_back(child);
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

            if (!child->tryDeleteSelf())
            {
                delete child;
            }
        }

        _childs.clear();
        _parent = nullptr;
        id = "";
        isDebugRectVisible = false;
        _state = STATE_NORMAL;
        _pivot = {0.0f, 0.0f};
        _localPosition = {0.0f, 0.0f};
        _localSize = {1.0f, 1.0f};
        _sizeInPercents = {-1, -1};
        _maxSize = {0, 0};
        _bgColor = RAYWHITE;
        _anchor = UI_LEFT_TOP;
        _layoutType = LAYOUT_NONE;
        _bounds = {0, 0, 1, 1};
    }

    void UiElement::drawDebugRect() const
    {
        auto x = std::to_string(_bounds.x);
        x.erase(x.find_last_not_of('0') + 1, std::string::npos);
        x.erase(x.find_last_not_of('.') + 1, std::string::npos);

        auto y = std::to_string(_bounds.y);
        y.erase(y.find_last_not_of('0') + 1, std::string::npos);
        y.erase(y.find_last_not_of('.') + 1, std::string::npos);

        auto width = std::to_string(_bounds.width);
        width.erase(width.find_last_not_of('0') + 1, std::string::npos);
        width.erase(width.find_last_not_of('.') + 1, std::string::npos);

        auto height = std::to_string(_bounds.height);
        height.erase(height.find_last_not_of('0') + 1, std::string::npos);
        height.erase(height.find_last_not_of('.') + 1, std::string::npos);
        const auto debugText = TextFormat("X:%s Y:%s W:%s H:%s", x.c_str(), y.c_str(), width.c_str(), height.c_str());
        GuiDummyRec(_bounds, debugText);

        if (_parent != nullptr)
        {
            DrawLine(_bounds.x, _bounds.y, _parent->getBounds().x, _parent->getBounds().y, RED);
            DrawLine(_bounds.x + _bounds.width, _bounds.y, _parent->getBounds().x + _parent->getBounds().width, _parent->getBounds().y, RED);
            DrawLine(_bounds.x, _bounds.y + _bounds.height, _parent->getBounds().x, _parent->getBounds().y + _parent->getBounds().height, RED);
            DrawLine(_bounds.x + _bounds.width, _bounds.y + _bounds.height, _parent->getBounds().x + _parent->getBounds().width, _parent->getBounds().y + _parent->getBounds().height, RED);
        }
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
        Vector2 computedSize = getComputedSize(effectiveParentBounds, prelimPosition);
        if (_maxSize.x > 0)
        {
            computedSize.x = std::min(computedSize.x, _maxSize.x);
        }

        if (_maxSize.y > 0)
        {
            computedSize.y = std::min(computedSize.y, _maxSize.y);
        }

        _bounds = {prelimPosition.x, prelimPosition.y, computedSize.x, computedSize.y};
        if (_layoutType == LAYOUT_NONE)
        {
            return;
        }

        const auto parentBounds = _parent ? _parent->getBounds() : Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        Vector2 currPos = {parentBounds.x, parentBounds.y};

        auto totalProp = 0.0f;
        for (const auto child: _childs)
        {
            totalProp += (_layoutType == LAYOUT_HORIZONTAL ? child->_sizeInPercents.x : child->_sizeInPercents.y);
        }

        if (totalProp == 0.0f) totalProp = 1.0f; // Avoid div0

        for (const auto child: _childs)
        {
            const auto prop = (_layoutType == LAYOUT_HORIZONTAL ? child->_sizeInPercents.x : child->_sizeInPercents.y) / totalProp;
            const auto childSize = (_layoutType == LAYOUT_HORIZONTAL ? Vector2{parentBounds.width * prop, parentBounds.height} : Vector2{parentBounds.width, parentBounds.height * prop});

            child->_bounds = {currPos.x, currPos.y, childSize.x, childSize.y};
            if (_layoutType == LAYOUT_HORIZONTAL) currPos.x += childSize.x;
            else currPos.y += childSize.y;
        }
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
