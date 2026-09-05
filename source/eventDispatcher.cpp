#include <gf/gf_module.h>
#include <gf/gf_scene.h>

#include "coreapi.hpp"
#include "eventDispatcher.hpp"
#include "events.hpp"
#include "hook.hpp"

namespace SyringeCore {
    void (*_setNextSceneOrig)(gfSceneManager* manager, char* name, int memLayout);

    static EventHandler* m_handlersHead = NULL;
    static EventHandler* m_handlersTail = NULL;

    void EventDispatcher::initializeEvents(CoreApi* api)
    {
        // Module load events
        api->syHookEx(0x80026db4, EventDispatcher::_moduleLoadedHook, OPT_SAVE_REGS | OPT_ORIG_PRE);
        api->syHookEx(0x800272e0, EventDispatcher::_moduleLoadedHook, OPT_SAVE_REGS | OPT_ORIG_PRE);

        // Module unload events
        api->syHookEx(0x800273d8, EventDispatcher::_moduleUnloadedHook, OPT_SAVE_REGS | OPT_ORIG_PRE);
        api->syHookEx(0x8002750c, EventDispatcher::_moduleUnloadedHook, OPT_SAVE_REGS | OPT_ORIG_PRE);

        // scene change event
        Hook* hook = api->syHookEx(0x8002d5ac, EventDispatcher::_setNextScene, OPT_DIRECT);
        hook->getTrampoline(reinterpret_cast<void**>(&_setNextSceneOrig));
    }

    void EventDispatcher::dispatchEvent(Event& event)
    {
        for (EventHandler* handler = m_handlersHead; handler != NULL; handler = handler->next)
        {
            if (handler->type == event.getType())
            {
                handler->func(event);
            }
        }
    }

    void EventDispatcher::subscribe(Event::EventType type, SyringeCore::EventHandlerFN func, s32 caller)
    {
        EventHandler* handler = new (Heaps::Syringe) EventHandler(type, func, caller);
        if (m_handlersTail != NULL)
        {
            m_handlersTail->next = handler;
        }
        else
        {
            m_handlersHead = handler;
        }
        m_handlersTail = handler;
    }

    void EventDispatcher::unsubscribe(s32 caller)
    {
        EventHandler* previous = NULL;
        EventHandler* handler = m_handlersHead;
        while (handler != NULL)
        {
            EventHandler* next = handler->next;
            if (handler->caller == caller)
            {
                if (previous != NULL)
                {
                    previous->next = next;
                }
                else
                {
                    m_handlersHead = next;
                }
                if (m_handlersTail == handler)
                {
                    m_handlersTail = previous;
                }
                delete handler;
            }
            else
            {
                previous = handler;
            }
            handler = next;
        }
    }

    void EventDispatcher::_moduleLoadedHook()
    {
        register gfModuleInfo* info;

        asm {
                mr info, r30
        }

        Event event;
        event.type = Event::ModuleLoad;
        event.payload.moduleInfo = info;
        EventDispatcher::dispatchEvent(event);
    }

    void EventDispatcher::_moduleUnloadedHook()
    {
        register gfModuleInfo* info;

        asm {
                mr info, r30
        }

        Event event;
        event.type = Event::ModuleUnload;
        event.payload.moduleInfo = info;
        EventDispatcher::dispatchEvent(event);
    }

    /**
     * Hook to dispatch scene change events.
     */
    void EventDispatcher::_setNextScene(gfSceneManager* manager, char* name, int memLayout)
    {
        // Call original first
        _setNextSceneOrig(manager, name, memLayout);

        Event event;
        event.type = Event::SceneChange;
        event.payload.sceneManager = manager;
        EventDispatcher::dispatchEvent(event);
    }
}
