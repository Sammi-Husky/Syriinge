#include "events/moduleLoadEvent.hpp"
#include "eventDispatcher.hpp"
#include <vector.h>

namespace SyringeCore {
    Vector<ModuleLoadCB> ModuleLoadEvent::callbacks;

    void ModuleLoadEvent::subscribe(ModuleLoadCB cb) { callbacks.push(cb); }
    void ModuleLoadEvent::dispatch(gfModuleInfo* info)
    {
        EventDispatcher::onModuleLoaded(info);
    }
}