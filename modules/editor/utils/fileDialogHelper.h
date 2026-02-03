#pragma once
#include <windows.h>
#include <string>

namespace BreadEditor {
    class FileDialogHelper
    {
    public:
        static std::wstring OpenFile(
            const HWND owner = nullptr,
            const wchar_t *filter = L"All Files (*.*)\0*.*\0",
            const wchar_t *title = L"Select File",
            const wchar_t *defExt = nullptr);

        static std::wstring SelectFolder(
            HWND owner = nullptr,
            const wchar_t *title = L"Select Folder");

        static std::string OpenFileUTF8(
            HWND owner = nullptr,
            const wchar_t *filter = L"All Files (*.*)\0*.*\0",
            const wchar_t *title = L"Select File",
            const wchar_t *defExt = nullptr);

        static std::string SelectFolderUTF8(
            HWND owner = nullptr,
            const wchar_t *title = L"Select Folder");

    private:
        static std::string WideToUtf8(const std::wstring &wstr);
    };
}
