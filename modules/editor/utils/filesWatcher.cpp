#define DMON_IMPL
#include "filesWatcher.h"

#include "dmon.h"

namespace BreadEditor {
    BreadEngine::Action<FilesWatcher::CallbackData> FilesWatcher::onFileChanged = BreadEngine::Action<CallbackData>();

    void watch_callback(dmon_watch_id watch_id, dmon_action action, const char *rootdir, const char *filepath, const char *oldfilepath, void *user)
    {
        const auto act = static_cast<FilesWatcher::FileAction>(action);
        FilesWatcher::onFileChanged.invoke(
            {act, filepath ? filepath : "", oldfilepath ? oldfilepath : "", rootdir ? rootdir : ""});
    }

    void FilesWatcher::start(const char *path)
    {
        dmon_init();
        dmon_watch(path, watch_callback, 0, nullptr);
    }

    void FilesWatcher::stop()
    {
        dmon_deinit();
        onFileChanged.unsubscribeAll();
    }
} // BreadEditor
