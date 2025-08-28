#include <FA.h>
#include <OS/OSError.h>

#include <gf/gf_module.h>
#include <stdio.h>
#include <vector.h>

#include "coreapi.hpp"
#include "events.hpp"
#include "hook.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"

namespace SyringeCore {
    CoreApi* API = NULL;
    Vector<Hook*> Injections;
    Vector<Syringe::Plugin*> Plugins;

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
        for (int i = 0; i < Injections.size(); i++)
        {
            applyInjection(Injections[i], header);
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
        bool isMelee = strcmp(sceneName, "scMelee") == 0;

        for (int i = 0; i < Plugins.size(); i++)
        {
            Syringe::Plugin* plg = Plugins[i];
            Syringe::PluginFlags flags = plg->getMetadata()->FLAGS;

            if (isMemoryChange && plg->getModule() != NULL)
            {
                if (flags.loading & Syringe::LOAD_PERSIST)
                    continue;

                plg->unload();
            }
            else if (isMelee && (flags.timing & Syringe::TIMING_MATCH))
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

        // subscribe to onModuleLoaded event to handle applying hooks
        API->EventManager.subscribe(Event::ModuleLoad, &onModuleLoaded, -1);

        // subscribe to onSceneChange event to handle loading plugins
        API->EventManager.subscribe(Event::SceneChange, &onSceneChange, -1);
    }

    bool faLoadPlugin(FAEntryInfo* info, const char* folder, s32 index)
    {
        char tmp[0x80];
        char* name = info->name[0] == 0 ? info->shortname : info->name;
        sprintf(tmp, "%s/%s", folder, name);

        Syringe::Plugin* plg = new (Heaps::Syringe) Syringe::Plugin(tmp, API, index);

        if (!plg->load())
        {
            OSReport("[Syringe] Failed to load plugin (%s)\n", tmp);
            return false;
        }

        // Unload the plugin after getting metadata if TIMING_BOOT is not set
        if (!(plg->getMetadata()->FLAGS.timing & Syringe::TIMING_BOOT))
        {
            plg->unload();
        }

        Plugins.push(plg);

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
