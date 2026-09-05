#include <FA.h>
#include <OS/OSError.h>

#include <gf/gf_module.h>
#include <string.h>

#include "coreapi.hpp"
#include "events.hpp"
#include "hash.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"

namespace SyringeCore {
    CoreApi* API = NULL;

    // Linked list of hooks. Tail is used so insertion is O(1) by not having to walk the full list to insert
    Hook* Hooks = NULL;
    Hook* HooksTail = NULL;

    // Linked list of plugins. Tail is used so insertion is O(1) by not having to walk the full list to insert
    Plugin* Plugins = NULL;
    Plugin* PluginsTail = NULL;

    /**
     * Determines if there is a trigger in the plugin metadata that matches the specified kind, action, and key.
     *
     * @param metadata The plugin metadata to search in.
     * @param kind The kind of trigger to find.
     * @param action The action of the trigger to find.
     * @param key The key associated with the trigger to find.
     * @return A pointer to the matching PluginTrigger if found, otherwise NULL.
     */
    static PluginTrigger* hasTrigger(PluginMeta* metadata, TriggerKind kind, TriggerAction action, u32 key)
    {
        for (int i = 0; i < MAX_LOAD_TRIGGERS; i++)
        {
            PluginTrigger* trigger = &metadata->LOAD_TRIGGERS[i];
            if (trigger->kind == kind && trigger->action == action && trigger->key == key)
            {
                return trigger;
            }
        }
        return NULL;
    }

    void applyInjection(Hook* hook, gfModuleHeader* header)
    {
        if (hook->getModuleId() != header->id)
            return;

        u32 address = hook->getTarget();

        // if this is a module hook, add offset to .text addr
        if (address < 0x80000000)
        {
            address += header->getTextSectionAddr();
        }

        // Apply the hook
        hook->apply(address);

        if (hook->getOptions() & OPT_DIRECT)
        {
            OSReport("[Syringe] Patching %8x -> %8x\n", hook->getInstalledAt(), hook->getDestination());
        }
        else
        {
            OSReport("[Syringe] Patching %8x -> %8x\n", hook->getInstalledAt(), (u32)hook->getPayloadAddr());
        }
    }

    void onModuleLoaded(Event& event)
    {
        if (event.getType() != Event::ModuleLoad)
            return;

        gfModuleInfo* info = event.getModuleInfo();
        gfModuleHeader* header = info->m_module->header;

        // Apply global hooks
        for (Hook* hook = Hooks; hook != NULL; hook = hook->getNext())
        {
            applyInjection(hook, header);
        }

        // Evaluate module load triggers: load any plugin that asked to piggyback
        // on this module's load.
        const char* moduleName = event.getModuleName();
        u32 nameHash = syHash(moduleName);

        for (Plugin* plg = Plugins; plg != NULL; plg = plg->getNext())
        {
            PluginMeta* meta = plg->getMetadata();

            if (plg->getModule() != NULL)
            {
                continue; // already loaded
            }

            PluginTrigger* trigger = hasTrigger(meta, TRIGGER_MODULE, TRIGGER_LOAD, nameHash);
            if (trigger != NULL)
            {
                if (trigger->heapSrc == HEAP_PIGGYBACK)
                {
                    // Load into the same heap the game used for this module
                    plg->loadIntoHeap(event.getHeap());
                }
                else
                {
                    // Load into the plugin's metadata heap
                    plg->load();
                }
                plg->execute();
            }
        }
    }

    void onModuleUnloaded(Event& event)
    {
        if (event.getType() != Event::ModuleUnload)
            return;

        u32 nameHash = syHash(event.getModuleName());

        for (Plugin* plg = Plugins; plg != NULL; plg = plg->getNext())
        {
            PluginMeta* meta = plg->getMetadata();

            if (plg->getModule() == NULL)
            {
                continue; // not loaded
            }

            if (hasTrigger(meta, TRIGGER_MODULE, TRIGGER_UNLOAD, nameHash) != NULL)
            {
                plg->unload();
            }
        }
    }

    void onSceneChange(Event& event)
    {
        if (event.getType() != Event::SceneChange)
            return;

        gfScene* scene = event.getNextScene();
        const char* sceneName = scene->m_sceneName;

        u32 sceneHash = syHash(sceneName);
        bool isMemoryChange = sceneHash == SY_SCMEMORYCHANGE_HASH;

        for (Plugin* plg = Plugins; plg != NULL; plg = plg->getNext())
        {
            PluginFlags flags = plg->getMetadata()->FLAGS;
            bool isLoaded = plg->getModule() != NULL;

            // Unload any non-persistent plugins during a memory change
            if (isMemoryChange && isLoaded)
            {
                // Persistent plugins survive memory changes.
                if (flags.loading == LOAD_PERSIST)
                {
                    continue;
                }

                plg->unload();
            }
            // If the plugin is already loaded and this isn't a memory change, don't
            // attempt to load it again
            else if (isLoaded && !isMemoryChange)
            {
                continue;
            }

            // At this point the plugin is unloaded, so check if a scene trigger
            // matches the current scene.
            if (hasTrigger(plg->getMetadata(), TRIGGER_SCENE, TRIGGER_LOAD, sceneHash) != NULL)
            {
                plg->load();
                plg->execute();
            }
        }
    }

    void syInit()
    {
        API = new (Heaps::Syringe) CoreApi();

        EventDispatcher::initializeEvents(API);

        // subscribe to onModuleLoaded event to handle applying hooks and module load triggers
        API->EventManager.subscribe(Event::ModuleLoad, &onModuleLoaded, -1);

        // subscribe to onModuleUnloaded to handle module unload triggers
        // NOTE: The game-side unload hook that dispatches ModuleUnloadEvent is not
        // yet wired (owner RE task). Once it dispatches, this handler is ready.
        API->EventManager.subscribe(Event::ModuleUnload, &onModuleUnloaded, -1);

        // subscribe to onSceneChange event to handle loading plugins
        API->EventManager.subscribe(Event::SceneChange, &onSceneChange, -1);
    }

    // Returns true if the plugin should be executed immediately at boot: either no
    // triggers were declared (first slot empty) or a scene trigger explicitly
    // requests "BOOT".
    static bool isBootPlugin(PluginMeta* meta)
    {
        // No triggers declared at all -> boot default (matches old behavior where
        // a NULL/empty timings array executed immediately).
        if (meta->LOAD_TRIGGERS[0].key == 0 && meta->LOAD_TRIGGERS[0].kind == TRIGGER_SCENE)
        {
            return true;
        }

        for (int i = 0; i < MAX_LOAD_TRIGGERS; i++)
        {
            PluginTrigger& t = meta->LOAD_TRIGGERS[i];
            if (t.kind == TRIGGER_SCENE && t.key == SY_BOOT_HASH)
            {
                return true;
            }
        }
        return false;
    }

    bool faLoadPlugin(FAEntryInfo* info, const char* folder, s32 index)
    {
        char tmp[0x80];
        char* name = info->name[0] == 0 ? info->shortname : info->name;

        // Skip hidden files
        if (name[0] == '.')
            return false;

        sprintf(tmp, "%s/%s", folder, name);

        Plugin* plg = new (Heaps::Syringe) Plugin(tmp, API, index);

        if (!plg->load())
        {
            OSReport("[Syringe] Failed to load plugin (%s)\n", tmp);
            return false;
        }

        // Save the created plugin
        if (PluginsTail != NULL)
        {
            PluginsTail->setNext(plg);
        }
        else
        {
            Plugins = plg;
        }
        PluginsTail = plg;

        PluginMeta* meta = plg->getMetadata();

        // If no triggers are declared or "BOOT" is requested, execute immediately.
        if (isBootPlugin(meta))
        {
            plg->execute();
            return true;
        }

        // Otherwise, unload the plugin for now
        plg->unload();

        return true;
    }
    int syLoadPlugins(const char* folder)
    {
        FAEntryInfo info;
        int count = 0;
        char tmp[0x80];
        sprintf(tmp, "%spf/%s/*.rel", MOD_PATCH_DIR, folder);
        if (FAFsfirst(tmp, 0x20, &info) == 0)
        {
            // Load first found plugin
            if (faLoadPlugin(&info, folder, count))
            {
                count++;
            }

            // Loop over and load the rest if there are more
            while (FAFsnext(&info) == 0)
            {
                if (faLoadPlugin(&info, folder, count))
                {
                    count++;
                }
            }
        }
        return count;
    }
} // namespace SyringeCore
