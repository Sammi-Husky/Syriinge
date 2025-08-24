#pragma once

#include <gf/gf_module.h>
#include <vector.h>

namespace SyringeCore {
    class CoreApi;
    namespace Events {
        enum EventType {
            MODULE_LOADED,
            // MODULE_UNLOADED
        };
        struct EventData {
            EventType eventType;
            void* argument;
        };

        typedef void (*EventCB)(Events::EventData*);
        struct Subscriber {
            Events::EventType eventType;
            EventCB callback;
        };

        static Vector<Subscriber> subscribers;

        void initializeEvents(CoreApi* API);

        class Event {
        public:
            /**
             * @brief Dispatches an event to all subscribers.
             * @param e Event data to dispatch
             */
            virtual void dispatch(EventData* e) = 0;
            /**
             * @brief Subscribes a callback to this event.
             * @param cb Callback function to register with the event
             */
            virtual void subscribe(EventCB cb) = 0;
            virtual ~Event() {};
        };

        class ModuleLoadEvent : public Event {
        public:
            /**
             * @brief Dispatches a module loaded event to all subscribers.
             * @param e Event data to dispatch
             */
            virtual void dispatch(EventData* e)
            {
                int num = subscribers.size();
                for (int i = 0; i < num; i++)
                {
                    if (subscribers[i].eventType == Events::MODULE_LOADED)
                        subscribers[i].callback(e);
                }
            }
            /**
             * @brief Subscribes a callback to this event.
             * @param cb Callback function to register with the event
             */
            virtual void subscribe(EventCB cb)
            {
                Subscriber sub;
                sub.eventType = Events::MODULE_LOADED;
                sub.callback = cb;
                subscribers.push(sub);
            }
        };
    }
}