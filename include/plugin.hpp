#pragma once

#include "sy_core.hpp"
#include <vector.h>
namespace SyringeCore {
    class CoreApi;
    class gfModule;
    class Hook;
}
namespace Syringe {
    class Plugin {
    private:
        char path[126];                   // Path to the plugin file
        PluginMeta* metadata;             // Pointer to the plugin metadata
        gfModule* module;                 // Pointer to the loaded module
        bool enable;                      // Indicates if the plugin is enabled
        SyringeCore::CoreApi* core;       // Pointer to the core API
        Vector<SyringeCore::Hook*> hooks; // Contains all hooks added by the plugin

    public:
        Plugin(const char* path, SyringeCore::CoreApi* api);

        /**
         * @brief Loads the plugin and returns a pointer to the module.
         */
        gfModule* loadPlugin();
        /**
         * @brief Unloads the plugin and restores original instructions for all hooks.
         */
        void unloadPlugin();
        /**
         * @brief Checks if the plugin is enabled.
         * @return True if the plugin is enabled, false otherwise.
         */
        bool isEnabled() { return enable; }
        /**
         * @brief Injects a hook at the target address.
         * @note Hooks injected via this function WILL automatically return execution to the original function.
         *
         * @param address address or offset to inject our hook at
         * @param function pointer to the function to run
         * @param moduleId (optional) ID of the target module, -1 for static hooks
         * @return Pointer to the created hook object
         */
        SyringeCore::Hook* addHook(const u32 address, const void* hook, int moduleId);
        /**
         * @brief Injects a hook at the target address with additional options.
         *
         * @param address address or offset to inject our hook at
         * @param function pointer to the function to inject
         * @param options options for the hook
         * @param moduleId (optional) ID of the target module
         * @returns pointer to the created hook
         */
        SyringeCore::Hook* addHookEx(const u32 address, const void* function, int options, int moduleId);
    };
}