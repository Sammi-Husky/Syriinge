#include <gf/gf_module.h>

#include "coreapi.h"
#include "events.h"
#include "hook.h"
#include <vector.h>

namespace SyringeCore {
    // TODO maybe split events into their own files?
    // This would reduce confusion around callback vectors
    Vector<ModuleLoadCB> ModuleLoadEvent::callbacks;

    void ModuleLoadEvent::init(CoreApi* api)
    {
        api->syHookEx(0x80026db4, reinterpret_cast<void*>(ModuleLoadEvent::process), OPT_SAVE_REGS | OPT_ORIG_PRE);
        api->syHookEx(0x800272e0, reinterpret_cast<void*>(ModuleLoadEvent::process), OPT_SAVE_REGS | OPT_ORIG_PRE);
    }

    void ModuleLoadEvent::process()
    {
        register gfModuleInfo* info;

        asm {
            mr info, r30
        }

        int numCB = callbacks.size();
        for (int i = 0; i < numCB; i++)
        {
            ModuleLoadEvent::callbacks[i](info);
        }
    }
}