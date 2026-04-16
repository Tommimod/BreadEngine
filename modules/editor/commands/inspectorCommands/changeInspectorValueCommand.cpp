#include "changeInspectorValueCommand.h"

namespace BreadEditor {
    ChangeInspectorValueCommand::ChangeInspectorValueCommand(Data data) : _data(std::move(data))
    {
        _isGenericData = true;
    }

    ChangeInspectorValueCommand::ChangeInspectorValueCommand(VectorData data) : _vectorData(std::move(data))
    {
    }

    ChangeInspectorValueCommand::~ChangeInspectorValueCommand() = default;

    void ChangeInspectorValueCommand::execute()
    {
        if (_isGenericData)
        {
            _data.property->set(_data.inspectorStruct, _data.value);
        }
        else
        {
            _vectorData.accessor->set(_vectorData.index, _vectorData.value);
        }
    }

    void ChangeInspectorValueCommand::undo()
    {
        if (_isGenericData)
        {
            _data.property->set(_data.inspectorStruct, _data.oldValue);
        }
        else
        {
            _vectorData.accessor->set(_vectorData.index, _vectorData.oldValue);
        }
    }
} // BreadEditor
