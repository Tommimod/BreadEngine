#include "mainToolbarSystem.h"
#include <ranges>
#include "commands/commandsHandler.h"
#include "commands/mainToolbarCommands/createNewProjectCommand.h"
#include "commands/mainToolbarCommands/openProjectCommand.h"

namespace BreadEditor {
    static std::set<MainToolbarSystem::ToolbarOption> empty;

    MainToolbarSystem::MainToolbarSystem()
    {
        _categoryToOptions =
        {
            {
                "File", std::set
                {
                    ToolbarOption{"New project", [] { CommandsHandler::execute(std::make_unique<CreateNewProjectCommand>()); }},
                    ToolbarOption{"Open project", [] { CommandsHandler::execute(std::make_unique<OpenProjectCommand>()); }},
                }
            }
        };
    }

    MainToolbarSystem::~MainToolbarSystem()
    = default;

    void MainToolbarSystem::addOption(const std::string &categoryKey, const ToolbarOption &option)
    {
        if (!categoryKey.contains(categoryKey))
        {
            _categoryToOptions.emplace(categoryKey, empty);
        }

        _categoryToOptions[categoryKey].emplace(option);
    }

    void MainToolbarSystem::addOptions(const std::string &categoryKey, const std::set<ToolbarOption> &options)
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

    void MainToolbarSystem::processCommand(const std::string &categoryKey, const int &optionIndex)
    {
        if (_categoryToOptions.contains(categoryKey))
        {
            std::ranges::next(_categoryToOptions[categoryKey].begin(), optionIndex - 1)->func();
        }
    }

    std::set<MainToolbarSystem::ToolbarOption> &MainToolbarSystem::getOptions(const std::string &categoryKey)
    {
        if (_categoryToOptions.contains(categoryKey))
        {
            return _categoryToOptions[categoryKey];
        }

        return empty;
    }

    std::vector<std::string> &MainToolbarSystem::getCategories()
    {
        _keys.clear();
        for (const auto &key: _categoryToOptions | std::views::keys)
        {
            _keys.push_back(key);
        }

        return _keys;
    }
} // BreadEditor
