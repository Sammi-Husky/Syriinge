#pragma once

#include "version.hpp"

#define DEPRECATE __attribute__((deprecated))

extern "C" char MOD_PATCH_DIR[0x18];

class gfModuleInfo;
namespace SyringeCore {
    class CoreApi;
}
namespace Syringe {
    class Plugin;
    enum LoadTiming {
        TIMING_BOOT = 1 << 0,        // Load the plugin when Syringe first loads
        TIMING_MAIN_MENU = 1 << 1,   // Load the plugin when the main menu is opened
        TIMING_CHAR_SELECT = 1 << 2, // Load the plugin when the character select screen is opened
        TIMING_MATCH = 1 << 3,       // Load the plugin when a match is started
        TIMING_SUBSPACE = 1 << 4     // Load the plugin when entering subspace
    };

    enum LoadType {
        LOAD_PERSIST = 1 << 0, // Persists plugin across scenes
        LOAD_UNLOAD = 1 << 1   // Unloads plugin when the scene changes
    };

    struct PluginFlags {
        u32 timing : 5;
        u32 loading : 2;
        u32 heap : 8;
        u32 _reserved : 17;
    };

    struct PluginMeta {
        char NAME[20];
        char AUTHOR[20];
        Version VERSION;
        Version SY_VERSION;
        void (*entrypoint)(Plugin* plg);
        PluginFlags FLAGS;
    };
} // namespace Syringe

namespace SyringeCore {
    /**
     * @brief Iterates over all loaded modules and attempts to apply registered hooks
     */
    // void hookLoadedModules();

    /**
     * @brief Initializes the Syringe core systems.
     * @note This function must be called before running any hooking operations.
     */
    void syInit();

    /**
     * @brief Loads plugins from the specified folder.
     * @param folder The folder to load plugins from.
     * @return The number of plugins loaded.
     */
    int syLoadPlugins(const char* folder);
}