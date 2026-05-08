#pragma once

namespace BreadEngine {
    class MandatoryProjectFilesValidator
    {
    public:
        [[nodiscard]] static bool validateAndInitialize();
    };
} // BreadEditor
