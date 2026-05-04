#pragma once
#include "action.h"
#include "uiElement.h"
#include "uiLabelButton.h"
#include "configs/asset.h"

namespace BreadEditor {
    class UiAssetLink : public UiElement
    {
    public:
        Action<Asset *> onClicked;
        Action<Asset *> onValueChanged;

        UiAssetLink() = default;

        ~UiAssetLink() override = default;

        UiAssetLink &setup(const std::string_view &id, Asset *asset);

        UiAssetLink &setup(const std::string_view &id, std::function<Asset *()> getFunc);

        UiAssetLink &setup(const std::string_view &id, UiElement *parentElement, Asset *asset);

        UiAssetLink &setup(const std::string_view &id, UiElement *parentElement, std::function<Asset *()> getFunc);

        void draw(float deltaTime) override;

        void update(float deltaTime) override;

        void dispose() override;

        void setAsset(Asset *asset);

        void setExpectedType(const std::type_index &typeIndex);

        [[nodiscard]] bool isAssetTypeValid(Asset *asset) const;

        [[nodiscard]] std::string getExpectedTypeName() const;

    protected:
        void awake() override;

        bool tryDeleteSelf() override;

    private:
        std::function<Asset *()> _getFunc;
        Asset *_asset = nullptr;
        Asset *_prevAsset = nullptr;
        UiLabelButton *_labelButton = nullptr;
        std::type_index _expectedType{typeid(void)};
        SubscriptionHandle _onDragEndedHandle;
        bool _isInitialized = false;

        void onDragEnded(UiElement *draggableElement);
    };
} // BreadEditor
