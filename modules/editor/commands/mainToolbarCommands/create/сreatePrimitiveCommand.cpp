#include "сreatePrimitiveCommand.h"

#include <magic_enum/magic_enum.hpp>

#include "createEmptyNodeCommand.h"
#include "editor.h"
#include "meshRenderer.h"
#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    CreatePrimitiveCommand::CreatePrimitiveCommand(Node *parentNode, const MeshPrimitiveType primitiveType)
    {
        _primitiveType = primitiveType;
        _parentNode = parentNode;
        if (_parentNode == nullptr)
        {
            _parentNode = &Engine::getRootNode();
        }
    }

    void CreatePrimitiveCommand::execute()
    {
        _nodeCreatedSubscription = NodeProvider::onNodeCreated.subscribe([this](Node *node) { onNodeCreated(node); });
        _fileSelectedSubscription = Editor::getInstance().getEditorModel().onFileSelected.subscribe([this](const FileUiElement *)
        {
            destroyInspector();
        });
        _nodeSelectedSubscription = Editor::getInstance().getEditorModel().onNodeSelected.subscribe([this](const Node *)
        {
            destroyInspector();
        });
        merge(std::make_unique<CreateEmptyNodeCommand>(_parentNode));
    }

    void CreatePrimitiveCommand::undo()
    {
        NodeProvider::onNodeCreated.unsubscribe(_nodeCreatedSubscription);
        Editor::getInstance().getEditorModel().onFileSelected.unsubscribe(_fileSelectedSubscription);
        Editor::getInstance().getEditorModel().onNodeSelected.unsubscribe(_nodeSelectedSubscription);
        destroyInspector();
        Command::undo();
    }

    void CreatePrimitiveCommand::onNodeCreated(Node *node)
    {
        NodeProvider::onNodeCreated.unsubscribe(_nodeCreatedSubscription);

        _data = createData(_primitiveType);
        if (_data == nullptr) return;

        node->setName(std::string(magic_enum::enum_name(_primitiveType)));
        _nodeId = node->getId();
        applyData();

        auto &viewportWindow = Editor::getInstance().mainWindow.getViewportWindow();
        _dataInspector = &UiPool::componentPool.get().setup("primitive_data_inspector", &viewportWindow, true, true);
        _dataInspector->setAnchor(UI_RIGHT_CENTER);
        _dataInspector->setPivot({1, 1});
        _dataInspector->setSize({0, 50});
        _dataInspector->setSizePercentPermanent({.2f, -1});
        _inspectorId = _dataInspector->id;
        _dataInspector->track(_data.get());
    }

    void CreatePrimitiveCommand::update()
    {
        if (_dataInspector == nullptr || _data == nullptr) return;

        if (IsKeyPressed(KEY_ESCAPE))
        {
            destroyInspector();
            return;
        }

        if (!_data->isChangedFromEditor) return;

        _data->isChangedFromEditor = false;
        applyData();
    }

    void CreatePrimitiveCommand::applyData() const
    {
        const auto node = NodeProvider::getNode(_nodeId);
        if (node == nullptr || !node->has<MeshRenderer>()) return;

        node->get<MeshRenderer>().setGeneratedMesh(*_data);
    }

    void CreatePrimitiveCommand::destroyInspector()
    {
        const auto &viewportWindow = Editor::getInstance().mainWindow.getViewportWindow();
        _dataInspector = static_cast<UiInspector *>(viewportWindow.getChildById(_inspectorId));
        if (_dataInspector != nullptr)
        {
            _dataInspector->getParentElement()->destroyChild(_dataInspector);
            _dataInspector = nullptr;
        }
    }

    std::unique_ptr<MeshPrimitiveData> CreatePrimitiveCommand::createData(const MeshPrimitiveType type)
    {
        switch (type)
        {
            case MeshPrimitiveType::Cube: return std::make_unique<CubePrimitiveData>();
            case MeshPrimitiveType::Sphere: return std::make_unique<SpherePrimitiveData>();
            case MeshPrimitiveType::HalfSphere:
            {
                auto data = std::make_unique<SpherePrimitiveData>();
                data->asHalf();
                return data;
            }
            case MeshPrimitiveType::Cylinder: return std::make_unique<CylinderPrimitiveData>();
            case MeshPrimitiveType::Capsule: return std::make_unique<CapsulePrimitiveData>();
            case MeshPrimitiveType::Plane: return std::make_unique<PlanePrimitiveData>();
            case MeshPrimitiveType::Quad:
            {
                auto data = std::make_unique<PlanePrimitiveData>();
                data->asQuad();
                return data;
            }
            case MeshPrimitiveType::None:
            default: return nullptr;
        }
    }
} // BreadEditor
