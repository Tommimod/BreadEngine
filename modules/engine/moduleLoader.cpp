#include "moduleLoader.h"

// Платформо-зависимые включения
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif

ModuleLoader::ModuleLoader() : _moduleHandle(nullptr)
{
}

ModuleLoader::~ModuleLoader()
{
    UnloadModule();
}

bool ModuleLoader::LoadModule(const std::string &path)
{
    UnloadModule();

    _moduleHandle = LoadLibrary(path);
    return _moduleHandle != nullptr;
}

void ModuleLoader::UnloadModule()
{
    if (_moduleHandle)
    {
        FreeLibrary(_moduleHandle);
        _moduleHandle = nullptr;
    }
}

void *ModuleLoader::GetFunction(const std::string &functionName)
{
    if (!_moduleHandle)
    {
        SetLastError("Module not loaded");
        return nullptr;
    }

    return GetProcAddress(_moduleHandle, functionName);
}

// Платформо-специфичные реализации
#ifdef _WIN32
void *ModuleLoader::LoadLibrary(const std::string &path)
{
    const HMODULE handle = ::LoadLibraryA(path.c_str());
    if (!handle)
    {
        const auto error = GetLastError();
        SetLastError("Failed to load library: " + error);
    }
    return handle;
}

void ModuleLoader::FreeLibrary(void *handle)
{
    ::FreeLibrary(static_cast<HMODULE>(handle));
}

void *ModuleLoader::GetProcAddress(void *handle, const std::string &functionName)
{
    const FARPROC proc = ::GetProcAddress(static_cast<HMODULE>(handle), functionName.c_str());
    if (!proc)
    {
        SetLastError("Function not found: " + functionName);
    }
    return reinterpret_cast<void *>(proc);
}

#elif defined(__APPLE__) || defined(__linux__)
// macOS/Linux реализация
void *ModuleLoader::LoadLibrary(const std::string &path)
{
    void *handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle)
    {
        const char *error = dlerror();
        SetLastError(error ? error : "Unknown error loading library");
    }
    return handle;
}

void ModuleLoader::FreeLibrary(void *handle)
{
    if (handle)
    {
        dlclose(handle);
    }
}

void *ModuleLoader::GetProcAddress(void *handle, const std::string &functionName)
{
    if (!handle) return nullptr;

    // Очищаем предыдущие ошибки
    dlerror();

    void *symbol = dlsym(handle, functionName.c_str());
    const char *error = dlerror();
    if (error)
    {
        SetLastError("Function not found: " + functionName + " (" + error + ")");
        return nullptr;
    }

    return symbol;
}

#else
// Неподдерживаемая платформа
void *ModuleLoader::LoadLibrary(const std::string &path)
{
    SetLastError("Dynamic loading not supported on this platform");
    return nullptr;
}

void ModuleLoader::FreeLibrary(void *handle)
{
    // Ничего не делаем
}

void *ModuleLoader::GetProcAddress(void *handle, const std::string &functionName)
{
    SetLastError("Dynamic loading not supported on this platform");
    return nullptr;
}
#endif

void ModuleLoader::SetLastError(const std::string &error)
{
    _lastError = error;
}
