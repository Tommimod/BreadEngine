#pragma once

#include <string>

// Кроссплатформенная система загрузки модулей
class ModuleLoader {
public:
    ModuleLoader();
    ~ModuleLoader();
    
    // Загрузить модуль
    bool LoadModule(const std::string& path);
    
    // Выгрузить модуль
    void UnloadModule();
    
    // Получить указатель на функцию
    void* GetFunction(const std::string& functionName);
    void *LoadLibraryA(const std::string &path);

    // Проверить, загружен ли модуль
    bool IsLoaded() const { return moduleHandle != nullptr; }
    
    // Получить последнюю ошибку
    std::string GetLastError() const { return lastError; }
    
private:
    void* moduleHandle;
    std::string lastError;
    
    // Платформо-специфичные методы
    void* LoadLibrary(const std::string& path);
    void FreeLibrary(void* handle);
    void* GetProcAddress(void* handle, const std::string& functionName);
    void SetLastError(const std::string& error);
};
