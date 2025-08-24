#pragma once

#include <gf/gf_module.h>
#include <vector.h>

namespace SyringeCore {
    class CoreApi;

    // Callback Types
    typedef void (*ModuleLoadCB)(gfModuleInfo*);

    class ModuleLoadEvent {
    public:
        static Vector<ModuleLoadCB> callbacks;
        static void subscribe(ModuleLoadCB cb) { callbacks.push(cb); }
        static void init(CoreApi* api);
        static void process();
    };
}