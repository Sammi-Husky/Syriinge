#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <vector.h>

#include "coreapi.h"
#include "events.h"
#include "hook.h"

namespace SyringeCore {
    extern Vector<Hook*> Injections;

    void** CoreApi::syHookEx(const u32 address, const void* function, int options, int moduleId)
    {
        Hook* hook = new (Heaps::Syringe) Hook(address,
                                               reinterpret_cast<u32>(function),
                                               moduleId,
                                               options);

        if (hook->type == HOOK_STATIC)
        {
            hook->apply(address);
            OSReport("[Syringe] Patching %8x -> %8x\n", address, (u32)function);
        }

        Injections.push(hook);

        return reinterpret_cast<void**>(&hook->trampoline);
    }
    void** CoreApi::syHook(const u32 address, const void* hook, int moduleId)
    {
        return CoreApi::syHookEx(address, hook, OPT_NONE, moduleId);
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
        original = syHookEx(address, replacement, OPT_DIRECT);
    }
    void CoreApi::syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        original = syHookEx(offset, replacement, OPT_DIRECT, moduleId);
    }
}