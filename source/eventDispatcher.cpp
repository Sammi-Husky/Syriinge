#include <gf/gf_module.h>

#include "coreapi.hpp"
#include "events.hpp"
#include "hook.hpp"
#include <vector.h>

namespace SyringeCore {
    void EventDispatcher::initializeEvents(CoreApi* api)
    {
        api->syHookEx(0x80026db4, reinterpret_cast<void*>(EventDispatcher::_moduleLoadedHook), OPT_SAVE_REGS | OPT_ORIG_PRE);
        api->syHookEx(0x800272e0, reinterpret_cast<void*>(EventDispatcher::_moduleLoadedHook), OPT_SAVE_REGS | OPT_ORIG_PRE);
    }

    void EventDispatcher::onModuleLoaded(gfModuleInfo* info)
    {
        int numCB = ModuleLoadEvent::callbacks.size();
        for (int i = 0; i < numCB; i++)
        {
            ModuleLoadEvent::callbacks[i](info);
        }
    }

    void EventDispatcher::_moduleLoadedHook()
    {
        register gfModuleInfo* info;

        asm {
                mr info, r30
        }

        EventDispatcher::onModuleLoaded(info);
    }
}