#include <gf/gf_module.h>
#include <gf/gf_scene.h>
#include <vector.h>

#include "coreapi.hpp"
#include "eventDispatcher.hpp"
#include "events.hpp"
#include "hook.hpp"

namespace SyringeCore {
    // Define the static event handler vector
    Vector<EventHandler> EventDispatcher::m_handlers;

    void EventDispatcher::initializeEvents(CoreApi* api)
    {
        // Module load events
        api->syHookEx(0x80026db4, reinterpret_cast<void*>(EventDispatcher::_moduleLoadedHook), OPT_SAVE_REGS | OPT_ORIG_PRE);
        api->syHookEx(0x800272e0, reinterpret_cast<void*>(EventDispatcher::_moduleLoadedHook), OPT_SAVE_REGS | OPT_ORIG_PRE);

        // scene change event
        api->syHookEx(0x8002d63c, reinterpret_cast<void*>(EventDispatcher::_setNextScene), OPT_DIRECT);
    }

    void EventDispatcher::dispatchEvent(Event& event)
    {
        int numCB = m_handlers.size();
        for (int i = 0; i < numCB; i++)
        {
            EventHandler handler = m_handlers[i];
            if (handler.type == event.getType())
            {
                handler.handler(event);
            }
        }
    }
    void EventDispatcher::subscribe(Event::EventType type, EventHandlerFN handler)
    {
        m_handlers.push(EventHandler(type, handler));
    }

    void EventDispatcher::_moduleLoadedHook()
    {
        register gfModuleInfo* info;

        asm {
                mr info, r30
        }

        ModuleLoadEvent event = ModuleLoadEvent(info);
        EventDispatcher::dispatchEvent(event);
    }

    /**
     * Hook to dispatch scene change events.
     */
    void EventDispatcher::_setNextScene(gfSceneManager* manager, char* name, int memLayout)
    {
        SceneChangeEvent event = SceneChangeEvent(manager);
        EventDispatcher::dispatchEvent(event);
    }
}