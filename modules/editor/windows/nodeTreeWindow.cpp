#include "nodeTreeWindow.h"

#include <thread>

#include "editor.h"
#include "engine.h"
#include "nodeProvider.h"
#include "raygui.h"
#include "uitoolkit/uiPool.h"

namespace BreadEditor {
    std::string NodeTreeWindow::Id = "Node Tree";

    NodeTreeWindow::NodeTreeWindow(const std::string &id) : UiWindow(id)
    {
        _editorModel = &Editor::getInstance().getEditorModel();
        setup(id);
        subscribe();
        rebuild();
    }

    NodeTreeWindow::NodeTreeWindow(const std::string &id, UiElement *parentElement) : UiWindow(id, parentElement)
    {
        _editorModel = &Editor::getInstance().getEditorModel();
        setup(id, parentElement);
        subscribe();
        rebuild();
    }

    NodeTreeWindow::~NodeTreeWindow() = default;

    void NodeTreeWindow::draw(const float deltaTime)
    {
        TraceLog(LOG_INFO, ("Size" + std::to_string(_bounds.width) + "x" + std::to_string(_bounds.height)).c_str());
        Editor::getInstance().setFontSize(static_cast<int>(EditorStyle::FontSize::MediumLarge));
        GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(EditorStyle::FontSize::MediumLarge));
        if (GuiScrollPanel(_bounds, nullptr, _contentView, &_scrollPos, &_scrollView)) //TODO Dirty*
        {
            setDirty();
        }

        UiWindow::draw(deltaTime);
        drawLines(Engine::getRootNode());
    }

    void NodeTreeWindow::update(const float deltaTime)
    {
        const auto selectedNode = _editorModel->getSelectedNodeUiElement();
        for (const auto nodeUiElement: _nodeUiElements)
        {
            nodeUiElement->setIsSelected(selectedNode != nullptr && selectedNode->id == nodeUiElement->id);
        }

        UiWindow::update(deltaTime);
    }

    void NodeTreeWindow::onFrameEnd(const float deltaTime)
    {
        UiWindow::onFrameEnd(deltaTime);
    }

    void NodeTreeWindow::dispose()
    {
        UiWindow::dispose();
    }

    NodeUiElement *NodeTreeWindow::getNodeUiElementByEngineNode(const Node *node) const
    {
        for (const auto child: getAllChilds())
        {
            if (const auto nodeInstance = dynamic_cast<NodeUiElement *>(child); nodeInstance && nodeInstance->getNode() == node)
            {
                return nodeInstance;
            }
        }

        return nullptr;
    }

    void NodeTreeWindow::rebuild()
    {
        const auto rootNode = &Engine::getRootNode();
        if (const auto instance = getNodeUiElementByEngineNode(rootNode); !instance)
        {
            rebuildByNode(rootNode);
        }
    }

    void NodeTreeWindow::rebuildByNode(Node *node)
    {
        if (const auto instance = getNodeUiElementByEngineNode(node); !instance)
        {
            onNodeCreated(node);
        }

        for (const auto child: node->getAllChilds())
        {
            rebuildByNode(child);
        }
    }

    void NodeTreeWindow::save()
    {
        if (!_isDirty) return;

        const auto filePath = Editor::getInstance().getConfigsProvider().getEditorPrefsConfig()->LastOpenedNodePath;
        auto _ = Engine::getRootNode().serialize(filePath);
        _isDirty = false;
    }

    void NodeTreeWindow::subscribe()
    {
        _nodeNotificatorSubscriptions.emplace_back(
            NodeProvider::onNodeCreated.subscribe([this](Node *node) { this->onNodeCreated(node); }));
        _nodeNotificatorSubscriptions.emplace_back(
            NodeProvider::onNodeChangedParent.subscribe([this](const Node *node) { this->onNodeChangedParent(node); }));
        _nodeNotificatorSubscriptions.emplace_back(
            NodeProvider::onNodeDestroyed.subscribe([this](const Node *node) { this->onNodeRemoved(node); }));
        _nodeNotificatorSubscriptions.emplace_back(
            NodeProvider::onNodeChangedActive.subscribe([this](const Node *node) { this->onNodeChangedActive(node); }));

        UiWindow::subscribe();
    }

    void NodeTreeWindow::unsubscribe()
    {
        NodeProvider::onNodeCreated.unsubscribe(_nodeNotificatorSubscriptions[0]);
        NodeProvider::onNodeChangedParent.unsubscribe(_nodeNotificatorSubscriptions[1]);
        NodeProvider::onNodeDestroyed.unsubscribe(_nodeNotificatorSubscriptions[2]);
        NodeProvider::onNodeChangedActive.unsubscribe(_nodeNotificatorSubscriptions[3]);
        _nodeNotificatorSubscriptions.clear();

        UiWindow::unsubscribe();
    }

    void NodeTreeWindow::onNodeCreated(Node *node)
    {
        constexpr float elementHeight = 20.0f;
        constexpr float elementWidthInPercent = 0.8f;

        update(0);

        const auto id = TextFormat(NodeUiElement::elementIdFormat, node->getName().c_str(), getChildCount());
        auto &element = UiPool::nodeUiElementPool.get().setup(id, this, node);

        element.setParentNode(getNodeUiElementByEngineNode(node->getParent()));
        element.setAnchor(UI_LEFT_TOP);
        element.setSize({0, elementHeight});
        element.setSizePercentPermanent({elementWidthInPercent, -1});
        element.dragContainer = _parent;
        element.onlyProvideDragEvents = true;

        _nodeUiElements.emplace_back(&element);
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
        std::vector<SubscriptionHandle> nodeSubscriptions{};
        element.onSelected.subscribe([](NodeUiElement *nodeUiElement) { onNodeSelected(nodeUiElement); });
        element.onDragStarted.subscribe([this](UiElement *uiElement) { onElementStartDrag(uiElement); });
        element.onDragEnded.subscribe([this](UiElement *uiElement) { onElementEndDrag(uiElement); });
        element.onExpandStateChanged.subscribe([this](NodeUiElement *)
        {
            int ii = 0;
            recalculateUiNodes(Engine::getRootNode(), ii);
        });
        element.onCopyRequested.subscribe([this](const YAML::Node &yamlData)
        {
            _copyData = yamlData;
        });
        element.onPasteRequested.subscribe([this](const NodeUiElement *nodeUiElement)
        {
            if (_copyData.IsNull()) return;
            Node::createCopyFromData(_copyData, *nodeUiElement->getNode());
            _copyData = {};
        });

        if (Editor::getInstance().isEditorMode())
        {
            _isDirty = true;
        }
    }

    void NodeTreeWindow::onNodeChangedParent(const Node *node)
    {
        const auto instance = getNodeUiElementByEngineNode(node);
        instance->setParentNode(getNodeUiElementByEngineNode(node->getParent()));

        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);

        if (Editor::getInstance().isEditorMode())
        {
            _isDirty = true;
        }
    }

    void NodeTreeWindow::onNodeChangedActive(const Node *node)
    {
        const auto instance = getNodeUiElementByEngineNode(node);
        instance->setState(node->getIsActive() ? STATE_NORMAL : STATE_DISABLED);

        if (Editor::getInstance().isEditorMode())
        {
            _isDirty = true;
        }
    }

    void NodeTreeWindow::onNodeRemoved(const Node *node)
    {
        const auto instance = getNodeUiElementByEngineNode(node);
        if (instance == nullptr) return;

        auto &model = Editor::getInstance().getEditorModel();
        model.selectNodeUiElement(nullptr);
        destroyChild(instance);
        _nodeUiElements.erase(ranges::find(_nodeUiElements, instance));
        if (_isDestroyProcessStarted) return;

        _isDestroyProcessStarted = true;
        int i = 0;
        recalculateUiNodes(Engine::getRootNode(), i);
        _isDestroyProcessStarted = false;

        if (Editor::getInstance().isEditorMode())
        {
            _isDirty = true;
        }
    }

    void NodeTreeWindow::onNodeSelected(NodeUiElement *nodeUiElement)
    {
        auto &model = Editor::getInstance().getEditorModel();
        model.selectNodeUiElement(nodeUiElement);
        Node *node = nullptr;
        if (nodeUiElement != nullptr)
        {
            node = nodeUiElement->getNode();
            Editor::getInstance().mainWindow.getGizmoSystem().recalculateGizmo(node->get<BreadEngine::Transform>());
        }

        PropertyInspectorWindow &nodeInspector = Editor::getInstance().mainWindow.getPropertyInspector();
        nodeInspector.lookupNode(node);
    }

    void NodeTreeWindow::onElementStartDrag(UiElement *uiElement)
    {
        const auto originalElement = dynamic_cast<NodeUiElement *>(uiElement);
        _draggedNodeUiElementCopy = originalElement->copySingle();
        _draggedNodeUiElementCopy->forceStartDrag(_draggedNodeUiElementCopy);
        originalElement->switchMuteState();
    }

    void NodeTreeWindow::onElementEndDrag(UiElement *uiElement)
    {
        if (_draggedNodeUiElementCopy == nullptr) return;

        destroyChild(_draggedNodeUiElementCopy);
        _draggedNodeUiElementCopy = nullptr;
        const auto originalElement = dynamic_cast<NodeUiElement *>(uiElement);
        originalElement->switchMuteState();
        for (const auto nodeElement: _nodeUiElements)
        {
            const auto nodeBounds = nodeElement->getBounds();
            if (Engine::isCollisionPointRec(GetMousePosition(), nodeBounds))
            {
                const auto currentNode = originalElement->getNode();
                const auto parentNode = nodeElement->getNode();
                if (currentNode->getParent() == nullptr || currentNode == parentNode)
                {
                    //avoid moving root or make self-parenting
                    return;
                }

                if (currentNode->isMyChild(parentNode))
                {
                    //can't be child of parent
                    return;
                }

                if (currentNode->getParent() == parentNode)
                {
                    parentNode->setChildFirst(currentNode);
                    int i = 0;
                    recalculateUiNodes(Engine::getRootNode(), i);
                    return;
                }

                currentNode->changeParent(parentNode);
                return;
            }
        }
    }

    void NodeTreeWindow::recalculateUiNodes(Node &startNode, int &nodeOrder, const bool isParentExpanded)
    {
        const auto element = getNodeUiElementByEngineNode(&startNode);
        if (!element) return;

        if (!isParentExpanded)
        {
            element->isActive = false;
            for (const auto child: startNode.getAllChilds())
            {
                recalculateUiNodes(*child, nodeOrder, false);
            }
            return;
        }

        element->isActive = true;
        element->computeBounds();
        element->update(0);
        constexpr float nodeHorizontalPadding = 15.0f;
        constexpr float nodeVerticalPadding = 5.0f;
        constexpr float startYPosition = 10;

        const auto nodeHeight = element->getSize().y;
        const auto deepLevel = static_cast<float>(startNode.getDeepLevel());

        const auto horizontalOffset = nodeHorizontalPadding * deepLevel;
        const auto verticalPadding = (nodeVerticalPadding + nodeHeight) * static_cast<float>(nodeOrder);
        element->setPosition({nodeHorizontalPadding + horizontalOffset, startYPosition + verticalPadding});
        nodeOrder++;
        calculateRectForScroll(element);
        if (startNode.getChildCount() == 0)
        {
            return;
        }

        const auto &isExpanded = element->getIsExpanded();
        for (const auto child: startNode.getAllChilds())
        {
            recalculateUiNodes(*child, nodeOrder, isExpanded);
        }
    }

    void NodeTreeWindow::drawLines(Node &startNode, const bool isParentExpanded) const
    {
        if (_nodeUiElements.empty()) return;

        constexpr float nodeHorizontalPadding = 15.0f * .5f;

        const auto instance = getNodeUiElementByEngineNode(&startNode);
        if (!instance || !isParentExpanded) return;

        const bool isLast = _nodeUiElements.back() == instance;
        const int childCount = instance->getNode()->getChildCount();
        const auto &isExpanded = instance->getIsExpanded();
        if (isLast || childCount == 0 || !isExpanded)
        {
            return;
        }

        const auto lineColor = instance->getState() == STATE_DISABLED ? GRAY : BLACK;
        const auto bounds = instance->getBounds();
        auto leftCenterPoint = Vector2{bounds.x, bounds.y + bounds.height * .5f};
        auto targetPoint = Vector2(leftCenterPoint);
        targetPoint.x -= nodeHorizontalPadding;
        DrawLineV(leftCenterPoint, targetPoint, lineColor);

        const auto childs = startNode.getAllChilds();
        const auto lastChild = childs.back();
        const auto lastElement = getNodeUiElementByEngineNode(lastChild);
        auto lastTargetPoint = Vector2(targetPoint);
        lastTargetPoint.y = 1 + lastElement->getBounds().y + lastElement->getBounds().height * .5f;
        targetPoint.y++;
        DrawLineV(targetPoint, lastTargetPoint, lineColor);

        for (const auto child: childs)
        {
            const auto childElement = getNodeUiElementByEngineNode(child);
            const auto childBound = childElement->getBounds();
            const auto xPoint = childBound.x;
            const auto yPoint = childBound.y + childBound.height * .5f;
            const auto target = Vector2(xPoint, yPoint);
            const auto from = Vector2{targetPoint.x, yPoint};
            DrawLineV(from, target, lineColor);
        }

        for (const auto child: childs)
        {
            drawLines(*child, isExpanded);
        }
    }
} // BreadEditor
