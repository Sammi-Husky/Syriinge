#pragma once

#include "events.hpp"

// Forward declarations
class gfSceneManager;
class gfModuleInfo;
class Event;
template <typename T>
class Vector;
namespace SyringeCore {
    class CoreApi;
}

namespace SyringeCore {
    typedef void (*EventHandlerFN)(Event& event);
    struct EventHandler {
        EventHandler() : type(Event::INVALID), handler(NULL) {}
        EventHandler(Event::EventType t, EventHandlerFN h) : type(t), handler(h) {}
        EventHandlerFN handler;
        Event::EventType type;
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
         * @param handler The function to call when the event is triggered.
         */
        static void subscribe(Event::EventType type, EventHandlerFN handler);

    private:
        static Vector<EventHandler> m_handlers;
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