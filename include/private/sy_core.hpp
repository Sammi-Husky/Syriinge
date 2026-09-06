#pragma once

#include "private/hook.hpp"
#include "version.hpp"
#include <gf/gf_module.h>

extern "C" char MOD_PATCH_DIR[0x18];

namespace SyringeCore {
    void syInit();
    int syLoadPlugins(const char* folder);
    void applyInjection(Hook* hook, gfModuleHeader* header);
}
