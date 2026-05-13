#include "mainToolbarSystem.h"
#include <ranges>
#include "commands/commandsHandler.h"
#include "../commands/mainToolbarCommands/file/createNewProjectCommand.h"
#include "../commands/mainToolbarCommands/file/openProjectCommand.h"
#include "../commands/mainToolbarCommands/file/saveProjectCommand.h"
#include "commands/mainToolbarCommands/create/createEmptyNodeCommand.h"

namespace BreadEditor {
    static std::set<MainToolbarSystem::ToolbarOption> empty;

    MainToolbarSystem::MainToolbarSystem()
    {
        _categoryToOptions.emplace("File", std::set
                                   {
                                       ToolbarOption{"New project", [] { CommandsHandler::execute(std::make_unique<CreateNewProjectCommand>()); }},
                                       ToolbarOption{"Open project", [] { CommandsHandler::execute(std::make_unique<OpenProjectCommand>()); }},
                                       ToolbarOption{"Save", [] { CommandsHandler::execute(std::make_unique<SaveProjectCommand>()); }}
                                   });
        _categoryToOptions.emplace("Create", std::set
                                   {
                                       ToolbarOption{
                                           "Add Empty", [] { CommandsHandler::execute(std::make_unique<CreateEmptyNodeCommand>(&BreadEngine::Engine::getRootNode())); }
                                       }
                                   });
    }

    MainToolbarSystem::~MainToolbarSystem()
    = default;

    void MainToolbarSystem::addOption(const std::string_view &categoryKey, const ToolbarOption &option)
    {
        if (!categoryKey.contains(categoryKey))
        {
            _categoryToOptions.emplace(categoryKey, empty);
        }

        _categoryToOptions[categoryKey].emplace(option);
    }

    void MainToolbarSystem::addOptions(const std::string_view &categoryKey, const std::set<ToolbarOption> &options)
    {
        if (!categoryKey.contains(categoryKey))
        {
            _categoryToOptions.emplace(categoryKey, empty);
        }

        for (const auto &option: options)
        {
            _categoryToOptions[categoryKey].emplace(option);
        }
    }

    void MainToolbarSystem::processCommand(const std::string_view &categoryKey, const int &optionIndex)
    {
        if (_categoryToOptions.contains(categoryKey))
        {
            std::ranges::next(_categoryToOptions[categoryKey].begin(), optionIndex - 1)->func();
        }
    }

    std::set<MainToolbarSystem::ToolbarOption> &MainToolbarSystem::getOptions(const std::string_view &categoryKey)
    {
        if (_categoryToOptions.contains(categoryKey))
        {
            return _categoryToOptions[categoryKey];
        }

        return empty;
    }

    std::vector<std::string_view> &MainToolbarSystem::getCategories()
    {
        _keys.clear();
        for (const auto &key: _categoryToOptions | std::views::keys)
        {
            _keys.push_back(key);
        }

        std::ranges::reverse(_keys);
        return _keys;
    }
} // BreadEditor
