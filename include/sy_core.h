#pragma once

#include "version.h"

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

enum HookType {
    HOOK_STATIC,
    HOOK_RELATIVE
};

enum HookOptions {
    OPT_NONE = 0x0,      // no options
    OPT_SAVE_REGS = 0x1, // use safe hook
    OPT_ORIG_PRE = 0x2,  // run the original instruction before hook
    OPT_ORIG_POST = 0x4, // run the original instruction after the hook
    OPT_NO_RETURN = 0x8, // do not return to hooked function
    OPT_DIRECT = 0x10    // use direct branch instead of wrapping in a trampoline
};

struct Trampoline {
    Trampoline(u32 originalInstr, u32 retAddr);
    u32 originalInstr; // original instruction
    u32 branch;        // branch to original func code + 4
};

class Hook {
public:
    HookType type;         // type of hook (static or relative)
    HookOptions options;   // hook options
    s8 moduleId;           // module ID this hook belongs to
    u32 tgtAddr;           // target address to hook
    u32 newAddr;           // address to branch to
    u32 instructions[14];  // hook instructions
    u32 originalInstr;     // original instruction at target address
    Trampoline trampoline; // trampoline to facilitate calling original function

    Hook(u32 source, u32 dest, s8 moduleId, int options);
    void apply(u32 address);

private:
    void setInstructions(u32 targetAddr, HookOptions opts);
};

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

    typedef void (*ModuleLoadCB)(gfModuleInfo*);
}
class CoreApi {
public:
    /**
     * @brief Injects a hook at the target address.
     * @note Hooks injected via this function WILL automatically return execution to the original function.
     *
     * @param address address to inject our hook at
     * @param replacement pointer to the function to run
     */
    virtual void syInlineHook(const u32 address, const void* replacement);
    /**
     * @brief Injects an inline hook into a dynamically loaded module on load.
     * @note Hooks injected via this function WILL automatically return execution to the original function.
     *
     * @param offset offset inside the module's .text section to insert the hook
     * @param replacement pointer to the function to inject
     * @param moduleId ID of the target module
     */
    virtual void syInlineHookRel(const u32 offset, const void* replacement, int moduleId);
    /**
     * @brief Injects a simple hook at the target address.
     * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
     *
     * @param address address to inject the hook at
     * @param replacement pointer to function the hook will point to
     */
    virtual void sySimpleHook(const u32 address, const void* replacement);
    /**
     * @brief Injects a simple hook into a dynamically loaded module on load.
     * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
     *
     * @param offset offset inside the module's .text section to insert the hook
     * @param replacement pointer to function the hook will point to
     * @param moduleId ID of the target module
     */
    virtual void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId);
    /**
     * @brief Replaces the function at the target address with the function pointed to by "replacement".
     * @note Replacement functions will not automatically call or return to the original function.
     * To call the original function, use the parameter "original"
     *
     * @param address address of the function to replace
     * @param replacement pointer to the replacement function
     * @param original pointer to the original function. Useful for calling the original behavior.
     */
    virtual void syReplaceFunc(const u32 address, const void* replacement, void** original);
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
    virtual void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId);

    /**
     *  @brief Registers an advanced hook at the target address.
     *  @note Advanced hooks allow for more complex behavior, such as whether to return to the original function,
     *  whether to run the original instruction, and whether to use a direct branch.
     *
     *  @param address address to inject the hook at
     *  @param replacement pointer to the function to run
     *  @param moduleId ID of the target module
     *  @param options options for the hook
     */
    virtual void syCustomHook(const u32 address, const void* replacement,
                              int options = 0,
                              int moduleId = -1);

    /**
     * @brief Registers a callback function that will be called whenever a module is loaded.
     */
    virtual void moduleLoadEventSubscribe(SyringeCore::ModuleLoadCB cb);
};