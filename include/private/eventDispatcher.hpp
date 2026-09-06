#pragma once

#include "events.hpp"

class gfSceneManager;
class gfModuleInfo;
class Event;

namespace SyringeCore {
    class CoreApi;

    struct EventHandler {
        EventHandler() : next(NULL), type(Event::INVALID), func(NULL), caller(-1) {}
        EventHandler(Event::EventType t, EventHandlerFN h, s32 c = -1) : next(NULL), type(t), func(h), caller(c) {}
        EventHandler* next;
        EventHandlerFN func;
        Event::EventType type;
        s32 caller;
    };

    class EventDispatcher {
    public:
        static void initializeEvents(CoreApi* API);
        static void dispatchEvent(Event& event);
        static void subscribe(Event::EventType type, EventHandlerFN func, s32 caller);
        static void subscribe(Event::EventType type, EventHandlerFN func);
        static void unsubscribe(s32 caller);

    private:
        static void _moduleLoadedHook();
        static void _moduleUnloadedHook();
        static void _setNextScene(gfSceneManager* manager, char* name, int memLayout);
    };
}
