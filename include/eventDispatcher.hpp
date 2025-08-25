#pragma once

#include <gf/gf_module.h>

namespace SyringeCore {
    class CoreApi;
    class EventDispatcher {
    public:
        /**
         * Initializes the event system.
         * @param API Pointer to the CoreApi instance.
         */
        static void initializeEvents(CoreApi* API);
        /**
         * Dispatches an onModuleLoaded event to all subscribers.
         * @param info Information about the loaded module.
         */
        static void onModuleLoaded(gfModuleInfo* info);

    private:
        /**
         * Hook for module loaded events.
         */
        static void _moduleLoadedHook();
    };
}
