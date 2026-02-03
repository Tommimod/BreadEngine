#include "fileDialogHelper.h"
#include <shlobj.h>
#include <windows.h>
namespace BreadEditor {
    std::wstring FileDialogHelper::OpenFile(const wchar_t *filter, const wchar_t *title, const wchar_t *defExt)
    {
        wchar_t path[MAX_PATH * 2] = {0};

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = path;
        ofn.nMaxFile = static_cast<DWORD>(_countof(path));
        ofn.lpstrTitle = title;
        ofn.lpstrDefExt = defExt;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileNameW(&ofn))
        {
            return path;
        }

        return std::wstring{};
    }

    std::wstring FileDialogHelper::SelectFolder(const wchar_t *title)
    {
        wchar_t path[MAX_PATH] = {0};

        BROWSEINFOW bi = {};
        bi.lpszTitle = title;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

        const LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (!pidl)
        {
            return std::wstring{};
        }

        bool success = SHGetPathFromIDListW(pidl, path) != FALSE;
        CoTaskMemFree(pidl);

        if (success)
        {
            return path;
        }

        return std::wstring{};
    }

    std::string FileDialogHelper::OpenFileUTF8(const wchar_t *filter, const wchar_t *title, const wchar_t *defExt)
    {
        const std::wstring wpath = OpenFile(filter, title, defExt);
        if (wpath.empty()) return {};

        return WideToUtf8(wpath);
    }

    std::string FileDialogHelper::SelectFolderUTF8(const wchar_t *title)
    {
        std::wstring wpath = SelectFolder(title);
        if (wpath.empty()) return {};

        return WideToUtf8(wpath);
    }

    std::string FileDialogHelper::WideToUtf8(const std::wstring &wstr)
    {
        if (wstr.empty()) return {};

        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return {};

        std::string utf8(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, utf8.data(), len, nullptr, nullptr);

        return utf8;
    }
} // BreadEditor
