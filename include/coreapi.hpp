#pragma once

#include "eventDispatcher.hpp"
#include <types.h>

#define DEPRECATE __attribute__((deprecated))

// Forward Declarations
namespace Syringe {
    class Plugin;
}
namespace SyringeCore {
    class Hook;
}
template <typename T>
class Vector;

namespace SyringeCore {
    class CoreApi {
    public:
        EventDispatcher EventManager;
        /**
         * @brief Injects a hook at the target address.
         * @note Hooks injected via this function WILL automatically return execution to the original function.
         *
         * @param address address or offset to inject our hook at
         * @param function pointer to the function to run
         * @param moduleId (optional) ID of the target module, -1 for static hooks
         * @param global (optional) whether the hook is added to the global hook list
         * @returns pointer to the created hook
         */
        virtual Hook* syHook(const u32 address, const void* function, int owner, int moduleId = -1);
        virtual Hook* syHook(const u32 address, const void* function, int moduleId = -1);
        /**
         * @brief Injects a hook at the target address with additional options.
         *
         * @param address address or offset to inject our hook at
         * @param function pointer to the function to inject
         * @param options options for the hook
         * @param moduleId (optional) ID of the target module
         * @param global (optional) whether the hook is added to the global hook list
         * @returns pointer to the created hook
         */
        virtual Hook* syHookEx(const u32 address, const void* function, int options, int owner, int moduleId = -1);
        virtual Hook* syHookEx(const u32 address, const void* function, int options, int moduleId = -1);
        /**
         * @brief Undoes all hooks owned by the specified plugin.
         *
         * @param owner ID of the plugin that owns the hooks (-1 for core)
         */
        virtual void undoHooksByOwner(int owner);
        /**
         * @brief Removes all hooks owned by the specified plugin.
         *
         * @param owner ID of the plugin that owns the hooks (-1 for core)
         */
        virtual void removeHooksByOwner(int owner);
        /**
         * @brief Gets the list of plugins registered with the system
         *
         * @returns pointer to the vector of registered plugins
         */
        virtual Vector<Syringe::Plugin*>* getRegisteredPlugins();
    };
}