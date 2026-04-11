#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <gf/gf_module.h>
#include <vector.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"

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
        else
        {
            const gfModuleInfo* moduleInfoArr = gfModuleManager::getInstance()->m_moduleInfos;
            for (u32 i = 0; i < 0x10; i++)
            {
                gfModule* currModule = moduleInfoArr[i].m_module;
                if (currModule != NULL)
                {
                    gfModuleHeader* currModuleHeader = currModule->header;
                    if (currModuleHeader->id == moduleId)
                    {
                        SyringeCore::applyInjection(hook, currModuleHeader);
                        break;
                    }
                }
            }
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
                Hook* hook = Hooks[i];
                hook->undo();
                Hooks.removeAt(i);
                delete hook;
                i--;
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