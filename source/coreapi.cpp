#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <gf/gf_module.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"
#include "sy_log.hpp"

namespace SyringeCore {
    // Linked list of hooks. Tail is used so insertion is O(1) by not having to walk the full list to insert
    extern Hook* Hooks;
    extern Hook* HooksTail;

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
            SY_LOG("[Syringe] Patching %8x -> %8x\n", address, (u32)function);
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

        if (HooksTail != NULL)
        {
            HooksTail->setNext(hook);
        }
        else
        {
            Hooks = hook;
        }
        HooksTail = hook;

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
        Hook* previous = NULL;
        Hook* hook = Hooks;
        while (hook != NULL)
        {
            Hook* next = hook->getNext();
            if (hook->getOwner() == owner)
            {
                hook->undo();
                if (previous != NULL)
                {
                    previous->setNext(next);
                }
                else
                {
                    Hooks = next;
                }
                if (HooksTail == hook)
                {
                    HooksTail = previous;
                }
                delete hook;
            }
            else
            {
                previous = hook;
            }
            hook = next;
        }
    }
    void CoreApi::undoHooksByOwner(int owner)
    {
        for (Hook* hook = Hooks; hook != NULL; hook = hook->getNext())
        {
            if (hook->getOwner() == owner)
            {
                hook->undo();
            }
        }
    }
}
