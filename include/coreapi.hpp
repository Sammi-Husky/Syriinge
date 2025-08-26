#pragma once

#include "events.hpp"
#include <types.h>

#define DEPRECATE __attribute__((deprecated))

namespace SyringeCore {
    class Hook;
    class CoreApi {
    public:
        ModuleLoadEvent onModuleLoaded;
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
        Hook* syHook(const u32 address, const void* function, int moduleId = -1, bool global = false);
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
        Hook* syHookEx(const u32 address, const void* function, int options, int moduleId = -1, bool global = false);
        /**
         * @brief Injects a hook at the target address.
         * @note Hooks injected via this function WILL automatically return execution to the original function.
         *
         * @param address address to inject our hook at
         * @param replacement pointer to the function to run
         */
        DEPRECATE void syInlineHook(const u32 address, const void* replacement);
        /**
         * @brief Injects an inline hook into a dynamically loaded module on load.
         * @note Hooks injected via this function WILL automatically return execution to the original function.
         *
         * @param offset offset inside the module's .text section to insert the hook
         * @param replacement pointer to the function to inject
         * @param moduleId ID of the target module
         */
        DEPRECATE void syInlineHookRel(const u32 offset, const void* replacement, int moduleId);
        /**
         * @brief Injects a simple hook at the target address.
         * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
         *
         * @param address address to inject the hook at
         * @param replacement pointer to function the hook will point to
         */
        DEPRECATE void sySimpleHook(const u32 address, const void* replacement);
        /**
         * @brief Injects a simple hook into a dynamically loaded module on load.
         * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
         *
         * @param offset offset inside the module's .text section to insert the hook
         * @param replacement pointer to function the hook will point to
         * @param moduleId ID of the target module
         */
        DEPRECATE void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId);
        /**
         * @brief Replaces the function at the target address with the function pointed to by "replacement".
         * @note Replacement functions will not automatically call or return to the original function.
         * To call the original function, use the parameter "original"
         *
         * @param address address of the function to replace
         * @param replacement pointer to the replacement function
         * @param original pointer to the original function. Useful for calling the original behavior.
         */
        DEPRECATE void syReplaceFunc(const u32 address, const void* replacement, void** original);
        /**
         * @brief Replaces a function inside of a dynamically loaded module on load.
         * @note Replacement functions will not automatically call or return to the original function.
         * To call the original function, use the parameter "original"
         *
         * @param offset offset inside the module's .text section of the function to replace
         * @param replacement pointer to the replacement function
         * @param original pointer to the original function. Useful for calling the original behavior.
         * @param moduleId ID of the target module
         */
        DEPRECATE void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId);
    };
}