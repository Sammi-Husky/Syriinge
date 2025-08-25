#pragma once

#include "version.hpp"

#define DEPRECATE __attribute__((deprecated))

extern "C" char MOD_PATCH_DIR[0x18];

class gfModuleInfo;
namespace Syringe {
    struct PluginMeta {
        char NAME[20];
        char AUTHOR[20];
        Version VERSION;
        Version SY_VERSION;
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