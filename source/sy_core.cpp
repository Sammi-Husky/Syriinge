#include <FA.h>
#include <OS/OSError.h>

#include <gf/gf_module.h>
#include <stdio.h>
#include <vector.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"

namespace SyringeCore {
    CoreApi* API = NULL;
    Vector<Hook*> Injections;
    Vector<Syringe::Plugin*> Plugins;

    void onModuleLoaded(gfModuleInfo* info)
    {
        gfModuleHeader* header = info->m_module->header;

        for (int i = 0; i < Injections.size(); i++)
        {
            Hook* inject = Injections[i];
            if (inject->getModuleId() != header->id)
                continue;

            u32 address = inject->getTarget();

            // if this is a module hook, add offset to .text addr
            if (address < 0x80000000)
                address += header->getTextSectionAddr();

            // Apply the hook
            inject->apply(address);

            if (inject->getOptions() & OPT_DIRECT)
            {
                OSReport("[Syringe] Patching %8x -> %8x\n", inject->getInstalledAt(), inject->getDestination());
            }
            else
            {
                OSReport("[Syringe] Patching %8x -> %8x\n", inject->getInstalledAt(), (u32)inject->getPayloadAddr());
            }
        }
    }

    void syInit()
    {
        API = new (Heaps::Syringe) CoreApi();

        EventDispatcher::initializeEvents(API);

        // subscribe to onModuleLoaded event to handle applying hooks
        API->onModuleLoaded.subscribe(onModuleLoaded);
    }

    bool faLoadPlugin(FAEntryInfo* info, const char* folder)
    {
        char tmp[0x80];
        char* name = info->name[0] == 0 ? info->shortname : info->name;
        sprintf(tmp, "%s/%s", folder, name);

        // TODO: Once we start tracking loaded plugins we
        // need to change this from stack to heap allocation
        Syringe::Plugin* plg = new (Heaps::Syringe) Syringe::Plugin(tmp, API);

        if (!plg->loadPlugin())
        {
            OSReport("[Syringe] Failed to load plugin (%s)\n", tmp);
            return false;
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
            if (faLoadPlugin(&info, folder))
                count++;

            // Loop over and load the rest if there are more
            while (FAFsnext(&info) == 0)
            {
                if (faLoadPlugin(&info, folder))
                    count++;
            }
        }
        return count;
    }
} // namespace SyringeCore
