#include <gf/gf_module.h>
#include <gf/gf_scene.h>
#include <vector.h>

#include "coreapi.hpp"
#include "eventDispatcher.hpp"
#include "events.hpp"
#include "hook.hpp"

namespace SyringeCore {
    void (*_setNextSceneOrig)(gfSceneManager* manager, char* name, int memLayout);

    // Vector containing all event handlers.
    // Store handlers by value to avoid a heap allocation per subscription.
    static Vector<EventHandler> m_handlers;

    void EventDispatcher::initializeEvents(CoreApi* api)
    {
        // Module load events
        api->syHookEx(0x80026db4, EventDispatcher::_moduleLoadedHook, OPT_SAVE_REGS | OPT_ORIG_PRE);
        api->syHookEx(0x800272e0, EventDispatcher::_moduleLoadedHook, OPT_SAVE_REGS | OPT_ORIG_PRE);

        // scene change event
        Hook* hook = api->syHookEx(0x8002d5ac, EventDispatcher::_setNextScene, OPT_DIRECT);
        hook->getTrampoline(reinterpret_cast<void**>(&_setNextSceneOrig));
    }

    void EventDispatcher::dispatchEvent(Event& event)
    {
        int numCB = m_handlers.size();
        for (int i = 0; i < numCB; i++)
        {
            EventHandler& handler = m_handlers[i];
            if (handler.type == event.getType())
            {
                handler.func(event);
            }
        }
    }

    void EventDispatcher::subscribe(Event::EventType type, SyringeCore::EventHandlerFN func, s32 caller)
    {
        m_handlers.push(EventHandler(type, func, caller));
    }

    void EventDispatcher::unsubscribe(s32 caller)
    {
        // Find any handlers by this caller and remove them
        for (int i = 0; i < m_handlers.size(); i++)
        {
            if (m_handlers[i].caller == caller)
            {
                m_handlers.removeAt(i);
                i--;
            }
        }
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
        // Call original first
        _setNextSceneOrig(manager, name, memLayout);

        SceneChangeEvent event = SceneChangeEvent(manager);
        EventDispatcher::dispatchEvent(event);
    }
}