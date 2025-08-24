#include <gf/gf_module.h>

#include "coreapi.h"
#include "events.h"
#include "hook.h"
#include <vector.h>

namespace SyringeCore {
    namespace Events {

        // Forward declarations
        void ModuleLoadedEvent();

        void initializeEvents(CoreApi* API)
        {
            API->syHookEx(0x80026db4, reinterpret_cast<void*>(ModuleLoadedEvent), OPT_SAVE_REGS | OPT_ORIG_PRE);
            API->syHookEx(0x800272e0, reinterpret_cast<void*>(ModuleLoadedEvent), OPT_SAVE_REGS | OPT_ORIG_PRE);
        }

        void ModuleLoadedEvent()
        {
            register gfModuleInfo* info;
            Events::EventData e;

            asm {
                mr info, r30
            }

            e.argument = info;
            e.eventType = Events::MODULE_LOADED;

            int num = subscribers.size();
            for (int i = 0; i < num; i++)
            {
                if (subscribers[i].eventType == Events::MODULE_LOADED)
                    subscribers[i].callback(&e);
            }
        }
    }
}