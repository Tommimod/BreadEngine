#pragma once
#include <functional>
#include <set>
#include <map>
#include <vector>

namespace BreadEditor {
    class MainToolbarSystem
    {
    public:
        struct ToolbarOption
        {
            std::string optionName;
            std::function<void ()> func;

            bool operator<(const ToolbarOption &other) const
            {
                return optionName < other.optionName;
            }

            bool operator>(const ToolbarOption &other) const
            {
                return optionName > other.optionName;
            }

            bool operator==(const ToolbarOption &other) const
            {
                return optionName == other.optionName;
            }

            bool operator!=(const ToolbarOption &other) const
            {
                return optionName != other.optionName;
            }
        };

        MainToolbarSystem();

        ~MainToolbarSystem();

        void addOption(const std::string &categoryKey, const ToolbarOption &option);

        void addOptions(const std::string &categoryKey, const std::set<ToolbarOption> &options);

        void processCommand(const std::string &categoryKey, const int &optionIndex);

        [[nodiscard]] std::set<ToolbarOption> &getOptions(const std::string &categoryKey);

        [[nodiscard]] std::vector<std::string> &getCategories();

    private:
        std::vector<std::string> _keys;
        std::map<std::string, std::set<ToolbarOption> > _categoryToOptions;
    };
} // BreadEditor
