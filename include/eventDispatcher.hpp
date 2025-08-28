#pragma once

#include "events.hpp"

// Forward declarations
class gfSceneManager;
class gfModuleInfo;
class Event;

namespace SyringeCore {
    class CoreApi;
}

namespace SyringeCore {
    typedef void (*EventHandlerFN)(Event& event);
    struct EventHandler {
        EventHandler() : type(Event::INVALID), func(NULL), caller(-1) {}
        EventHandler(Event::EventType t, EventHandlerFN h, s32 c = -1) : type(t), func(h), caller(c) {}
        EventHandlerFN func;
        Event::EventType type;
        s32 caller; // ID of the subscriber, -1 if not applicable
    };

    class EventDispatcher {
    public:
        /**
         * Initializes the event system.
         * @param API Pointer to the CoreApi instance.
         */
        static void initializeEvents(CoreApi* API);
        /**
         * Dispatches an event to all subscribers.
         * @param event The event to dispatch.
         */
        static void dispatchEvent(Event& event);
        /**
         * Subscribes to an event.
         * @param type The type of event to subscribe to.
         * @param func The function to call when the event is triggered.
         * @param caller The ID of the subscriber.
         */
        static void subscribe(Event::EventType type, EventHandlerFN func, s32 caller);
        /**
         * Subscribes to an event.
         * @param type The type of event to subscribe to.
         * @param func The function to call when the event is triggered.
         */
        static void subscribe(Event::EventType type, EventHandlerFN func);
        /**
         * Unsubscribes from all events for a specific caller.
         * @param caller The ID of the subscriber.
         */
        static void unsubscribe(s32 caller);

    private:
        /**
         * Hook for module loaded events.
         */
        static void _moduleLoadedHook();
        /**
         * Hook for managing scene related events
         */
        static void _setNextScene(gfSceneManager* manager, char* name, int memLayout);
    };
}