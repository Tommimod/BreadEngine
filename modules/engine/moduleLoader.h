#pragma once

#include <string>

// Кроссплатформенная система загрузки модулей
class ModuleLoader
{
public:
    ModuleLoader();

    ~ModuleLoader();

    // Загрузить модуль
    bool LoadModule(const std::string &path);

    // Выгрузить модуль
    void UnloadModule();

    // Получить указатель на функцию
    void *GetFunction(const std::string &functionName);

    void *LoadLibraryA(const std::string &path);

    [[nodiscard]] bool IsLoaded() const { return _moduleHandle != nullptr; }

    [[nodiscard]] std::string GetLastError() const { return _lastError; }

private:
    void *_moduleHandle;
    std::string _lastError;

    void *LoadLibrary(const std::string &path);

    void FreeLibrary(void *handle);

    void *GetProcAddress(void *handle, const std::string &functionName);

    void SetLastError(const std::string &error);
};
