#include "uiAssetLink.h"

#include "editor.h"
#include "uiPool.h"

namespace BreadEditor {
    UiAssetLink &UiAssetLink::setup(const std::string_view &id, Asset *asset)
    {
        UiElement::setup(id);
        _asset = asset;
        return *this;
    }

    UiAssetLink &UiAssetLink::setup(const std::string_view &id, std::function<Asset *()> getFunc)
    {
        UiElement::setup(id);
        _getFunc = std::move(getFunc);
        return *this;
    }

    UiAssetLink &UiAssetLink::setup(const std::string_view &id, UiElement *parentElement, Asset *asset)
    {
        UiElement::setup(id, parentElement);
        _asset = asset;
        return *this;
    }

    UiAssetLink &UiAssetLink::setup(const std::string_view &id, UiElement *parentElement, std::function<Asset *()> getFunc)
    {
        UiElement::setup(id, parentElement);
        _getFunc = std::move(getFunc);
        return *this;
    }

    void UiAssetLink::awake()
    {
        _labelButton = &UiPool::labelButtonPool.get().setup(id + "_label", this, "");
        _labelButton->setSizePercentPermanent({1, 1});
        _labelButton->onClick.subscribe([this](UiLabelButton *) { onClicked.invoke(_asset); });

        _onDragEndedHandle = Editor::getInstance().getEditorModel().onDragEnded.subscribe([this](UiElement *element) { onDragEnded(element); });
    }

    void UiAssetLink::draw(float deltaTime)
    {
        GuiPanel(_bounds, nullptr);
    }

    void UiAssetLink::update(float deltaTime)
    {
        const auto isMouseOver = Engine::isCollisionPointRec(GetMousePosition(), _bounds);
        _labelButton->setState(isMouseOver || _asset != nullptr ? STATE_NORMAL : STATE_DISABLED);

        if (_getFunc != nullptr)
        {
            _asset = _getFunc();
        }

        if (_prevAsset == _asset && _isInitialized) return;
        if (_asset != nullptr)
        {
            _labelButton->setText(GuiIconText(ICON_FILE, TextFormat("%s (%s)", _asset->getAssetName().c_str(), getExpectedTypeName().c_str())));
        }
        else
        {
            _labelButton->setText(TextFormat(" None (%s)", getExpectedTypeName().c_str()));
        }

        _prevAsset = _asset;
        _isInitialized = true;
    }

    void UiAssetLink::dispose()
    {
        _isInitialized = false;
        _getFunc = nullptr;
        _asset = nullptr;
        _prevAsset = nullptr;
        _labelButton = nullptr;
        onValueChanged.unsubscribeAll();
        onClicked.unsubscribeAll();
        Editor::getInstance().getEditorModel().onDragEnded.unsubscribe(_onDragEndedHandle);
        UiElement::dispose();
    }

    void UiAssetLink::setExpectedType(const std::type_index &typeIndex)
    {
        _expectedType = typeIndex;
    }

    std::string UiAssetLink::getExpectedTypeName() const
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

    bool UiAssetLink::tryDeleteSelf()
    {
        UiPool::assetLinkPool.release(*this);
        return true;
    }

    void UiAssetLink::onDragEnded(UiElement *draggableElement)
    {
        const auto isMouseOver = Engine::isCollisionPointRec(GetMousePosition(), _bounds);
        if (!isMouseOver || draggableElement == nullptr) return;
        if (const auto fileUiElement = dynamic_cast<FileUiElement *>(draggableElement); fileUiElement != nullptr && _expectedType != typeid(void))
        {
            const auto &guid = fileUiElement->getFileGuid();
            auto &assetConfig = Engine::getInstance().getAssetsConfig();
            const auto file = assetConfig.getFileByGuid(guid);
            if (const auto asset = assetConfig.getAsset(file).get(); isAssetTypeValid(asset))
            {
                setAsset(asset);
            }
        }
    }

    void UiAssetLink::setAsset(Asset *asset)
    {
        _asset = asset;
        onValueChanged.invoke(_asset);
    }

    bool UiAssetLink::isAssetTypeValid(Asset *asset) const
    {
        if (asset == nullptr) return true;
        if (_expectedType == typeid(void)) return true;
        return std::type_index(typeid(*asset)) == _expectedType;
    }
} // BreadEditor
