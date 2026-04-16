#pragma once
#include "inspectorObject.h"
#include "commands/command.h"

namespace BreadEditor {
    struct ChangeInspectorValueCommand : Command
    {
        struct Data
        {
            BreadEngine::Property *property = nullptr;
            BreadEngine::InspectorStruct *inspectorStruct = nullptr;
            std::any value;
            std::any oldValue;

            Data() = default;

            Data(const BreadEngine::Property &property, BreadEngine::InspectorStruct *inspectorStruct, const std::any &value, const std::any &oldValue)
            {
                this->property = const_cast<BreadEngine::Property *>(&property);
                this->inspectorStruct = inspectorStruct;
                this->value = std::move(value);
                this->oldValue = std::move(oldValue);
            }

            ~Data() = default;
        };

        struct VectorData
        {
            BreadEngine::VectorAccessor *accessor = nullptr;
            int index = -1;
            std::any value;
            std::any oldValue;

            VectorData() = default;

            VectorData(BreadEngine::VectorAccessor *accessor, const int &index, const std::any &value, const std::any &oldValue)
            {
                this->accessor = accessor;
                this->index = index;
                this->value = std::move(value);
                this->oldValue = std::move(oldValue);
            }

            ~VectorData() = default;
        };

        explicit ChangeInspectorValueCommand(Data data);

        explicit ChangeInspectorValueCommand(VectorData data);

        ~ChangeInspectorValueCommand() override;

    private:
        VectorData _vectorData;
        Data _data;
        bool _isGenericData = false;

        bool withUndo() override { return true; }

        void execute() override;

        void undo() override;
    };
} // BreadEditor
