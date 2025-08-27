#pragma once

#include "sy_core.hpp"
#include <vector.h>
namespace SyringeCore {
    class CoreApi;
    class Hook;
}
class gfModule;
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
        ~Plugin();

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
         * @brief Gets the core API instance.
         * @warning The hooking functions contained in the core API are much lower level than the ones
         * provided by the Plugin class. Only use them if you know what you are doing or risk instability.
         * For example, hooks injected via the core API will not be automatically be tied to the plugin
         * and must manually be added to the plugin's hook list. Otherwise, unloading the plugin will not
         * disable those hooks.
         * @return Pointer to the core API instance.
         */
        SyringeCore::CoreApi* getCoreApi() { return core; }
        /**
         * @brief Gets the plugin metadata.
         * @return Pointer to the plugin metadata.
         */
        PluginMeta* getMetadata() { return metadata; }
        /**
         * @brief Injects a hook at the target address and registers it with the plugin
         * @note Hooks injected via this function WILL automatically return execution to the original function.
         *
         * @param address address or offset to inject our hook at
         * @param function pointer to the function to run
         * @param moduleId (optional) ID of the target module, -1 for static hooks
         * @return Pointer to the created hook object
         */
        SyringeCore::Hook* addHook(const u32 address, const void* hook, int moduleId);
        /**
         * @brief Injects a hook at the target address with additional options and registers it with the plugin.
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