#include "fileDialogHelper.h"

#include <iostream>
#include <shlobj.h>
#include <windows.h>

namespace BreadEditor {
    std::wstring FileDialogHelper::OpenFile(const wchar_t *filter, const wchar_t *title, const wchar_t *defExt)
    {
        wchar_t path[MAX_PATH * 4] = {0};

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = path;
        ofn.nMaxFile = _countof(path);
        ofn.lpstrTitle = title;
        ofn.lpstrDefExt = defExt;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

        std::wstring initialDir = GetExecutableDirectory();
        if (!initialDir.empty())
        {
            ofn.lpstrInitialDir = initialDir.c_str();
        }

        if (GetOpenFileNameW(&ofn)) return path;

        return {};
    }

    std::wstring FileDialogHelper::SelectFolder(const wchar_t *title)
    {
        wchar_t path[MAX_PATH * 4] = {0};

        std::wstring initialDir = GetExecutableDirectory(); // твоя функция, возвращает wstring

        BROWSEINFOW bi = {};
        bi.hwndOwner = nullptr; // или GetActiveWindow(), если нужно
        bi.lpszTitle = title;
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
        bi.lpfn = [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
        {
            switch (uMsg)
            {
                case BFFM_INITIALIZED:
                    // lpData — это указатель на wide-строку с начальным путём
                    if (lpData)
                    {
                        SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
                    }
                    break;
            }
            return 0;
        };
        bi.lParam = reinterpret_cast<LPARAM>(initialDir.c_str()); // передаём путь

        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (!pidl)
        {
            return {};
        }

        bool success = SHGetPathFromIDListW(pidl, path) != FALSE;
        CoTaskMemFree(pidl);

        return success ? std::wstring(path) : std::wstring{};
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

        std::wcout << L"Raw wide path from SHBrowse: [" << wstr << L"]\n";
        int len = WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.data(),
            static_cast<int>(wstr.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (len <= 0) return {};

        std::string utf8(len, '\0');

        int result = WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.data(),
            static_cast<int>(wstr.size()),
            utf8.data(),
            len,
            nullptr,
            nullptr
        );
        std::cout << "UTF-8 result: [" << utf8 << "]\n";
        std::cout << "UTF-8 length: " << utf8.size() << "\n";
        if (result == 0) return {};
        return utf8;
    }

    std::wstring FileDialogHelper::GetExecutableDirectory()
    {
        wchar_t exePath[MAX_PATH * 2] = {0};
        DWORD len = GetModuleFileNameW(nullptr, exePath, _countof(exePath));
        if (len == 0 || len >= _countof(exePath)) return {};

        // Убираем имя файла, оставляем только путь к папке
        wchar_t *lastSlash = wcsrchr(exePath, L'\\');
        if (lastSlash) *lastSlash = L'\0';

        return exePath;
    }
} // BreadEditor
