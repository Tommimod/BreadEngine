#include "uiNodeLink.h"
#include <utility>
#include <cxxabi.h>

#include "cameraDirector.h"
#include "editor.h"
#include "engine.h"
#include "uiPool.h"

namespace BreadEditor {
    UiNodeLink &UiNodeLink::setup(const std::string_view &id, Component *component)
    {
        _component = component;
        UiElement::setup(id);
        return *this;
    }

    UiNodeLink &UiNodeLink::setup(const std::string_view &id, std::function<Component *()> getFunc)
    {
        _getFunc = std::move(getFunc);
        UiElement::setup(id);
        return *this;
    }

    UiNodeLink &UiNodeLink::setup(const std::string_view &id, UiElement *parentElement, Component *component)
    {
        _component = component;
        UiElement::setup(id, parentElement);
        return *this;
    }

    UiNodeLink &UiNodeLink::setup(const std::string_view &id, UiElement *parentElement, std::function<Component *()> getFunc)
    {
        _getFunc = std::move(getFunc);
        UiElement::setup(id, parentElement);
        return *this;
    }

    void UiNodeLink::awake()
    {
        _labelButton = &UiPool::labelButtonPool.get().setup(id + "_label", this, "");
        _labelButton->setSizePercentPermanent({1, 1});
        _labelButton->onClick.subscribe([this](UiLabelButton *) { onClicked.invoke(_component); });

        _onDragEndedHandle = Editor::getInstance().getEditorModel().onDragEnded.subscribe([this](UiElement *element) { onDragEnded(element); });
    }

    void UiNodeLink::draw(float deltaTime)
    {
        GuiPanel(_bounds, nullptr);
    }

    void UiNodeLink::update(float deltaTime)
    {
        const auto isMouseOver = Engine::isCollisionPointRec(GetMousePosition(), _bounds);
        _labelButton->setState(isMouseOver || _component != nullptr ? STATE_NORMAL : STATE_DISABLED);

        if (_getFunc != nullptr)
        {
            _component = _getFunc();
        }

        if (_prevComponent == _component && _isInitialized) return;
        if (_component != nullptr)
        {
            _labelButton->setText(GuiIconText(ICON_CPU, TextFormat("%s (%s)", _component->getOwner()->getName().c_str(), _component->getTypeName().c_str())));
        }
        else
        {
            _labelButton->setText(TextFormat(" None (%s)", getExpectedTypeName().c_str()));
        }

        _prevComponent = _component;
        _isInitialized = true;
    }

    void UiNodeLink::dispose()
    {
        _isInitialized = false;
        _getFunc = nullptr;
        _component = nullptr;
        _prevComponent = nullptr;
        _labelButton = nullptr;
        onValueChanged.unsubscribeAll();
        onClicked.unsubscribeAll();
        Editor::getInstance().getEditorModel().onDragEnded.unsubscribe(_onDragEndedHandle);
        UiElement::dispose();
    }

    void UiNodeLink::setComponent(Component *component)
    {
        onValueChanged.invoke(component);
    }

    void UiNodeLink::setExpectedType(const std::type_index &typeIndex)
    {
        _expectedType = typeIndex;
    }

    bool UiNodeLink::isComponentTypeValid(Component *component) const
    {
        if (component == nullptr) return true;
        if (_expectedType == typeid(void)) return true;
        return std::type_index(typeid(*component)) == _expectedType;
    }

    std::string UiNodeLink::getExpectedTypeName() const
    {
        constexpr std::string empty = "";
        if (_expectedType == typeid(void)) return empty;
        int status;
        char *demangled = abi::__cxa_demangle(_expectedType.name(), nullptr, nullptr, &status);
        std::string result = status == 0 && demangled ? demangled : _expectedType.name();
        free(demangled);
        if (const size_t lastColon = result.rfind("::"); lastColon != std::string::npos)
        {
            return result.substr(lastColon + 2);
        }
        return result;
    }

    bool UiNodeLink::tryDeleteSelf()
    {
        UiPool::nodeLinkPool.release(*this);
        return true;
    }

    void UiNodeLink::onDragEnded(UiElement *draggableElement)
    {
        const auto isMouseOver = Engine::isCollisionPointRec(GetMousePosition(), _bounds);
        if (!isMouseOver || draggableElement == nullptr) return;
        if (const auto nodeUiElement = dynamic_cast<NodeUiElement *>(draggableElement); nodeUiElement != nullptr && _expectedType != typeid(void))
        {
            const auto *node = nodeUiElement->getNode();
            if (const auto component = ComponentsProvider::get(node->getId(), _expectedType); isComponentTypeValid(component))
            {
                setComponent(component);
            }
        }
    }
} // BreadEditor
