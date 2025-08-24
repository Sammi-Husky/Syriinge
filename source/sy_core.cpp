#include <FA.h>
#include <OS/OSError.h>

#include <gf/gf_module.h>
#include <stdio.h>
#include <vector.h>

#include "coreapi.h"
#include "hook.h"
#include "plugin.h"
#include "sy_core.h"

namespace SyringeCore {
    CoreApi* API = NULL;
    Vector<Hook*> Injections;

    void onModuleLoaded(Events::EventData* e)
    {
        gfModuleInfo* info = static_cast<gfModuleInfo*>(e->argument);

        gfModuleHeader* header = info->m_module->header;

        u32 textAddr = header->getTextSectionAddr();
        int numInjections = Injections.size();

        for (int i = 0; i < numInjections; i++)
        {
            Hook* inject = Injections[i];
            if (inject->moduleId != header->id)
            {
                continue;
            }

            u32 targetAddr = inject->tgtAddr;

            // if this is a module hook, add offset to .text addr
            if (targetAddr < 0x80000000)
                targetAddr += textAddr;

            if (inject->options & OPT_DIRECT)
            {
                OSReport("[Syringe] Patching %8x -> %8x\n", targetAddr, inject->newAddr);
            }
            else
            {
                OSReport("[Syringe] Patching %8x -> %8x\n", targetAddr, (u32)&inject->instructions[0]);
            }

            inject->apply(targetAddr);
        }
    }

    void syInit()
    {
        API = new (Heaps::Syringe) CoreApi();

        // Initialize event system
        Events::initializeEvents(API);

        // Subscribe to the onModuleLoaded event to handle hooking rels
        API->onModuleLoaded.subscribe(onModuleLoaded);
    }

    bool faLoadPlugin(FAEntryInfo* info, const char* folder)
    {
        char tmp[0x80];
        if (info->name[0] == 0)
            sprintf(tmp, "%s/%s", folder, info->shortname);
        else
            sprintf(tmp, "%s/%s", folder, info->name);

        Syringe::Plugin plg = Syringe::Plugin(tmp);

        if (!plg.loadPlugin(API))
        {
            OSReport("[Syringe] Failed to load plugin (%s)\n", tmp);
            return false;
        }

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
