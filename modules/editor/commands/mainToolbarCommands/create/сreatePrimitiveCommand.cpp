#include "сreatePrimitiveCommand.h"

#include "createEmptyNodeCommand.h"
#include "editor.h"
#include "meshRenderer.h"
#include "commands/commandsHandler.h"
#include "data/primitives/capsulePrimitiveData.h"
#include "data/primitives/cubePrimitiveData.h"
#include "data/primitives/cylinderPrimitiveData.h"
#include "data/primitives/freePolyPrimitiveData.h"
#include "data/primitives/planePrimitiveData.h"
#include "data/primitives/slopePrimitiveData.h"
#include "data/primitives/spherePrimitiveData.h"
#include "data/primitives/torusPrimitiveData.h"
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
        if (_primitiveType == MeshPrimitiveType::None) return;

        auto &viewportWindow = Editor::getInstance().mainWindow.getViewportWindow();
        _dataInspector = &UiPool::componentPool.get().setup("primitive_data_inspector", &viewportWindow, true, true);
        _dataInspector->setAnchor(UI_RIGHT_CENTER);
        _dataInspector->setPivot({1, 1});
        _dataInspector->setSize({0, 50});
        _dataInspector->setSizePercentPermanent({.2f, -1});
        _inspectorId = _dataInspector->id;

        switch (_primitiveType)
        {
            case MeshPrimitiveType::Cube:
            {
                node->setName("Cube");
                auto data = CubePrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Sphere:
            {
                node->setName("Sphere");
                auto data = SpherePrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::HalfSphere:
            {
                node->setName("HalfSphere");
                auto data = SpherePrimitiveData();
                data.asHalf();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Cylinder:
            {
                node->setName("Cylinder");
                auto data = CylinderPrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Capsule:
            {
                node->setName("Capsule");
                auto data = CapsulePrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Plane:
            {
                node->setName("Plane");
                auto data = PlanePrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Quad:
            {
                node->setName("Quad");
                auto data = PlanePrimitiveData();
                data.asQuad();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Slope:
            {
                node->setName("Slope");
                auto data = SlopePrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::Torus:
            {
                node->setName("Torus");
                auto data = TorusPrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::FreePoly:
            {
                node->setName("FreePoly Mesh");
                auto data = FreePolyPrimitiveData();
                getFunctionByType(node->getId(), data)();

                const auto id = node->getId();
                auto func = [this, id, data]
                {
                    auto localData = data;
                    processDataAsync(id, localData);
                };
                _thread = std::thread(func);
                break;
            }
            case MeshPrimitiveType::None:
            default: break;
        }

        _thread.detach();
    }

    void CreatePrimitiveCommand::processDataAsync(const unsigned int nodeId, MeshPrimitiveData &data)
    {
        const auto link = &data;
        _dataInspector->track(link);
        while (_dataInspector != nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (IsKeyPressed(KEY_ESCAPE))
            {
                _dataInspector->getParentElement()->destroyChild(_dataInspector);
                _dataInspector = nullptr;
            }

            if (link->isChangedFromEditor)
            {
                auto func = getFunctionByType(nodeId, *link);
                CommandsHandler::addFunction(std::move(func));
                link->isChangedFromEditor = false;
            }
        }
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

    std::function<void()> CreatePrimitiveCommand::getFunctionByType(const unsigned int nodeId, MeshPrimitiveData &data)
    {
        if (data.getMeshType() == MeshPrimitiveType::None) return nullptr;

        auto &meshRenderer = NodeProvider::getNode(nodeId)->get<MeshRenderer>();
        return [&meshRenderer, &data] { meshRenderer.setGeneratedMesh(data); };
    }
} // BreadEditor
