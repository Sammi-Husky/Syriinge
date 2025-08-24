#pragma once

#include "sy_core.h"
#include <gf/gf_module.h>

namespace SyringeCore {
    class CoreApi;
}
namespace Syringe {
    class Plugin {
    private:
        char path[126];
        PluginMeta* metadata;
        gfModule* module;
        bool enable;
        // Vector<plgParam*> settings;

    public:
        Plugin(const char* path);
        gfModule* loadPlugin(SyringeCore::CoreApi* api);
        void unloadPlugin();
        bool isEnabled() { return enable; }
        // plgParam* getParam(const char* name);
        // void setParam(const char* name, plgParam* param);
    };
}