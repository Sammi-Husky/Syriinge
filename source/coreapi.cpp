#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <vector.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"

namespace SyringeCore {
    // Global Hook List. Used for internal / core hooks.
    extern Vector<Hook*> Hooks;

    // Global Plugin List.
    extern Vector<Plugin*> Plugins;

    Hook* CoreApi::syHookEx(const u32 address, const void* function, int options, int owner, int moduleId)
    {
        Hook* hook = new (Heaps::Syringe) Hook(address,
                                               reinterpret_cast<u32>(function),
                                               moduleId,
                                               options,
                                               owner);

        if (hook->getType() == HOOK_STATIC)
        {
            hook->apply(address);
            OSReport("[Syringe] Patching %8x -> %8x\n", address, (u32)function);
        }

        Hooks.push(hook);

        return hook;
    }
    Hook* CoreApi::syHookEx(const u32 address, const void* function, int options, int moduleId)
    {
        return CoreApi::syHookEx(address, function, options, -1, moduleId);
    }
    Hook* CoreApi::syHook(const u32 address, const void* hook, int owner, int moduleId)
    {
        return CoreApi::syHookEx(address, hook, OPT_NONE, owner, moduleId);
    }
    Hook* CoreApi::syHook(const u32 address, const void* hook, int moduleId)
    {
        return CoreApi::syHookEx(address, hook, OPT_NONE, -1, moduleId);
    }

    void CoreApi::removeHooksByOwner(int owner)
    {
        for (int i = 0; i < Hooks.size(); i++)
        {
            if (Hooks[i]->getOwner() == owner)
            {
                Hooks[i]->undo();
                Hooks.removeAt(i);
            }
        }
    }
    void CoreApi::undoHooksByOwner(int owner)
    {
        for (int i = 0; i < Hooks.size(); i++)
        {
            if (Hooks[i]->getOwner() == owner)
            {
                Hooks[i]->undo();
            }
        }
    }

    Vector<Plugin*>* CoreApi::getRegisteredPlugins()
    {
        return &Plugins;
    }
}