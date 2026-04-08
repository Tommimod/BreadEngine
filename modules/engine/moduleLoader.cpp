#include "moduleLoader.h"

#include <thread>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif

ModuleLoader::ModuleLoader() : _moduleHandle(nullptr)
{
}

ModuleLoader::~ModuleLoader() = default;

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

#ifdef _WIN32
void *ModuleLoader::LoadLibrary(const std::string &path)
{
    const DWORD fileAttrs = GetFileAttributesA(path.c_str());
    if (fileAttrs == INVALID_FILE_ATTRIBUTES)
    {
        this->SetLastError("File not found: " + path);
        return nullptr;
    }

    const HMODULE handle = ::LoadLibraryA(path.c_str());
    if (!handle)
    {
        const DWORD errorCode = ::GetLastError();
        LPSTR messageBuffer = nullptr;
        const size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorCode, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, nullptr);

        std::string errorMsg = "Failed to load library (code " + std::to_string(errorCode) + "): ";
        if (size > 0 && messageBuffer)
        {
            errorMsg += std::string(messageBuffer, size);
            LocalFree(messageBuffer);
        }
        else
        {
            errorMsg += "Unknown error";
        }
        this->SetLastError(errorMsg);
    }
    return handle;
}

void ModuleLoader::FreeLibrary(void *handle)
{
    if (handle)
    {
        ::FreeLibrary(static_cast<HMODULE>(handle));
    }
}

void *ModuleLoader::GetProcAddress(void *handle, const std::string &functionName)
{
    const FARPROC proc = ::GetProcAddress(static_cast<HMODULE>(handle), functionName.c_str());
    if (!proc)
    {
        this->SetLastError("Function not found: " + functionName);
    }
    return reinterpret_cast<void *>(proc);
}

#elif defined(__APPLE__) || defined(__linux__)

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

void *ModuleLoader::LoadLibrary(const std::string &path)
{
    SetLastError("Dynamic loading not supported on this platform");
    return nullptr;
}

void ModuleLoader::FreeLibrary(void *handle)
{
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
