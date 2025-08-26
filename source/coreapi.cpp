#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <vector.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"

namespace SyringeCore {
    // Global Hook List. Used for internal / core hooks.
    extern Vector<Hook*> Injections;

    // Global Plugin List.
    extern Vector<Syringe::Plugin*> Plugins;

    Hook* CoreApi::syHookEx(const u32 address, const void* function, int options, bool global, int moduleId)
    {
        Hook* hook = new (Heaps::Syringe) Hook(address,
                                               reinterpret_cast<u32>(function),
                                               moduleId,
                                               options);

        if (hook->getType() == HOOK_STATIC)
        {
            hook->apply(address);
            OSReport("[Syringe] Patching %8x -> %8x\n", address, (u32)function);
        }

        // If this is a global hook, add it to the global hook list
        if (global)
        {
            Injections.push(hook);
        }

        return hook;
    }
    Hook* CoreApi::syHookEx(const u32 address, const void* function, int options, int moduleId)
    {
        return CoreApi::syHookEx(address, function, options, true, moduleId);
    }
    Hook* CoreApi::syHook(const u32 address, const void* hook, bool global, int moduleId)
    {
        return CoreApi::syHookEx(address, hook, OPT_NONE, moduleId, global);
    }
    Hook* CoreApi::syHook(const u32 address, const void* hook, int moduleId)
    {
        return CoreApi::syHookEx(address, hook, OPT_NONE, true, moduleId);
    }
    void CoreApi::syInlineHook(const u32 address, const void* replacement)
    {
        syHookEx(address, replacement, OPT_SAVE_REGS | OPT_ORIG_PRE);
    }
    void CoreApi::syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        syHookEx(offset, replacement, OPT_SAVE_REGS | OPT_ORIG_PRE, moduleId);
    }
    void CoreApi::sySimpleHook(const u32 address, const void* replacement)
    {
        syHookEx(address, replacement, OPT_DIRECT);
    }
    void CoreApi::sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        syHookEx(offset, replacement, OPT_DIRECT, moduleId);
    }
    void CoreApi::syReplaceFunc(const u32 address, const void* replacement, void** original)
    {
        Hook* hook = syHookEx(address, replacement, OPT_DIRECT);
        original = hook->getTrampoline();
    }
    void CoreApi::syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        Hook* hook = syHookEx(offset, replacement, OPT_DIRECT, moduleId);
        original = hook->getTrampoline();
    }
    Vector<Syringe::Plugin*>* CoreApi::getRegisteredPlugins()
    {
        return &Plugins;
    }
}