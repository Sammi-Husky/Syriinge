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

    void onModuleLoaded(gfModuleInfo* info)
    {
        gfModuleHeader* header = info->m_module->header;

        for (int i = 0; i < Injections.size(); i++)
        {
            Hook* inject = Injections[i];
            if (inject->moduleId != header->id)
                continue;

            u32 targetAddr = inject->tgtAddr;

            // if this is a module hook, add offset to .text addr
            if (targetAddr < 0x80000000)
                targetAddr += header->getTextSectionAddr();

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
