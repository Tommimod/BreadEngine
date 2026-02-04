#pragma once

namespace BreadEditor {
    class MandatoryFilesValidator
    {
    public:
        MandatoryFilesValidator();

        ~MandatoryFilesValidator();

        [[nodiscard]] bool validate();
    };
} // BreadEditor
