#pragma once
#include <string>

namespace BreadEditor {
    class FileDialogHelper
    {
    public:
        [[nodiscard]] static std::wstring OpenFile(
            const wchar_t *filter = L"All Files (*.*)\0*.*\0",
            const wchar_t *title = L"Select File",
            const wchar_t *defExt = nullptr);

        [[nodiscard]] static std::wstring SelectFolder(
            const wchar_t *title = L"Select Folder");

        [[nodiscard]] static std::string OpenFileUTF8(
            const wchar_t *filter = L"All Files (*.*)\0*.*\0",
            const wchar_t *title = L"Select File",
            const wchar_t *defExt = nullptr);

        [[nodiscard]] static std::string SelectFolderUTF8(
            const wchar_t *title = L"Select Folder");

    private:
        [[nodiscard]] static std::string WideToUtf8(const std::wstring &wstr);

        [[nodiscard]] static std::wstring GetExecutableDirectory();
    };
}
