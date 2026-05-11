#pragma once
#include "action.h"

namespace BreadEditor {
    class FilesWatcher
    {
    public:
        typedef enum FileAction
        {
            ACTION_CREATE = 1,
            ACTION_DELETE,
            ACTION_MODIFY,
            ACTION_MOVE
        } FileAction;

        struct CallbackData
        {
            FileAction action;
            std::string path;
            std::string oldPath;
            std::string rootdir;
        };

        static BreadEngine::Action<CallbackData> onFileChanged;

        static void start(const char *path);

        static void stop();
    };
} // BreadEditor
