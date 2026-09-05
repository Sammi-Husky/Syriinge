#pragma once

#include "events.hpp"

namespace SyringeCore {
    class Hook;
}

class Plugin {
public:
    virtual SyringeCore::Hook* addHook(const u32 address, const void* hook, int moduleId = -1) = 0;
    virtual SyringeCore::Hook* addHookEx(const u32 address, const void* function, int options, int moduleId = -1) = 0;
    virtual void addEventHandler(Event::EventType type, SyringeCore::EventHandlerFN func) = 0;
    virtual void clearEventHandlers() = 0;
};
