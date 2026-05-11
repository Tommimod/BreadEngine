#pragma once
#include "filesWatcher.h"

namespace BreadEditor {
    class AssetsConfigUpdater
    {
    public:
        static void subscribe();

        static void unsubscribe();

    private:
        static BreadEngine::SubscriptionHandle _subscription;

        static void onUpdate(const FilesWatcher::CallbackData &data);
    };
} // BreadEditor
