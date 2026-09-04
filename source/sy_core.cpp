#include <FA.h>
#include <OS/OSError.h>

#include <gf/gf_module.h>
#include <string.h>
#include <vector.h>

#include "coreapi.hpp"
#include "events.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"

namespace SyringeCore {
    CoreApi* API = NULL;
    Vector<Hook*> Hooks;
    Vector<Plugin*> Plugins;

    void applyInjection(Hook* hook, gfModuleHeader* header)
    {
        if (hook->getModuleId() != header->id)
            return;

        u32 address = hook->getTarget();

        // if this is a module hook, add offset to .text addr
        if (address < 0x80000000)
            address += header->getTextSectionAddr();

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

        ModuleLoadEvent& moduleEvent = static_cast<ModuleLoadEvent&>(event);

        gfModuleInfo* info = moduleEvent.getModuleInfo();
        gfModuleHeader* header = info->m_module->header;

        // Apply global hooks
        for (int i = 0; i < Hooks.size(); i++)
        {
            applyInjection(Hooks[i], header);
        }
    }

    void onSceneChange(Event& event)
    {
        if (event.getType() != Event::SceneChange)
            return;

        SceneChangeEvent& sceneEvent = static_cast<SceneChangeEvent&>(event);
        gfScene* scene = sceneEvent.getNextScene();
        const char* sceneName = scene->m_sceneName;

        bool isMemoryChange = strcmp(sceneName, "scMemoryChange") == 0;

        for (u8 i = 0; i < Plugins.size(); i++)
        {
            Plugin* plg = Plugins[i];
            PluginFlags flags = plg->getMetadata()->FLAGS;
            const char** loadTimings = plg->getMetadata()->LOAD_TIMINGS;
            bool isLoaded = plg->getModule() != NULL;

            if (loadTimings == NULL || loadTimings[0] == NULL)
                continue;

            // Unload any non-persistent plugins during a memory change
            if (isMemoryChange && isLoaded)
            {
                if (flags.loading & LOAD_PERSIST)
                    continue;

                plg->unload();
            }
            // If the plugin is already loaded and this isn't memory change, don't attempt to load it again
            else if (isLoaded && !isMemoryChange)
            {
                continue;
            }

            // At this point we've determined the plugin is unloaded so check if it should be loaded for the current scene
            for (int x = 0; x < MAX_LOAD_TRIGGERS; x++)
            {
                if (loadTimings[x] == NULL)
                    continue;

                if (stricmp(loadTimings[x], sceneName) == 0)
                {
                    plg->load();
                    plg->execute();
                    break;
                }
            }
        }
    }

    void syInit()
    {
        API = new (Heaps::Syringe) CoreApi();

        EventDispatcher::initializeEvents(API);

        // subscribe to onModuleLoaded event to handle applying hooks
        API->EventManager.subscribe(Event::ModuleLoad, &onModuleLoaded, -1);

        // subscribe to onSceneChange event to handle loading plugins
        API->EventManager.subscribe(Event::SceneChange, &onSceneChange, -1);
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
        Plugins.push(plg);

        PluginMeta* meta = plg->getMetadata();

        // if LOAD_TIMINGS is NULL or the first timing is "BOOT", execute the plugin immediately
        if (meta->LOAD_TIMINGS == NULL || meta->LOAD_TIMINGS[0] == NULL || stricmp(meta->LOAD_TIMINGS[0], "BOOT") == 0)
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
                count++;

            // Loop over and load the rest if there are more
            while (FAFsnext(&info) == 0)
            {
                if (faLoadPlugin(&info, folder, count))
                    count++;
            }
        }
        return count;
    }
} // namespace SyringeCore
