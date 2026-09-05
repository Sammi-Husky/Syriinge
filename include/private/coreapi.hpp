#pragma once

#include "private/eventDispatcher.hpp"
#include <types.h>

namespace SyringeCore {
    class Hook;

    class CoreApi {
    public:
        EventDispatcher EventManager;
        virtual Hook* syHook(const u32 address, const void* function, int owner, int moduleId = -1);
        virtual Hook* syHook(const u32 address, const void* function, int moduleId = -1);
        virtual Hook* syHookEx(const u32 address, const void* function, int options, int owner, int moduleId = -1);
        virtual Hook* syHookEx(const u32 address, const void* function, int options, int moduleId = -1);
        virtual void undoHooksByOwner(int owner);
        virtual void removeHooksByOwner(int owner);
    };
}
