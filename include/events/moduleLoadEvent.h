#pragma once

#include <gf/gf_module.h>
#include <vector.h>

namespace SyringeCore {

    typedef void (*ModuleLoadCB)(gfModuleInfo*);

    class ModuleLoadEvent {
    private:
        static Vector<ModuleLoadCB> callbacks;
        friend class EventDispatcher;

    public:
        void subscribe(ModuleLoadCB cb);
        void dispatch(gfModuleInfo* info);
    };
}
