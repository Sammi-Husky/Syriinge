#include <FA.h>
#include <OS/OSCache.h>
#include <OS/OSError.h>
#include <gf/gf_module.h>
#include <stdio.h>
#include <vector.h>

#include "coreapi.h"
#include "plugin.h"
#include "sy_core.h"
#include "sy_utils.h"

namespace SyringeCore {
    static CoreApi* API = NULL;
    static Vector<Hook*> Injections;
    // Vector<Syringe::Plugin*> Plugins;
    
    namespace ModuleLoadEvent {
        
        Vector<ModuleLoadCB> Callbacks;
        void Subscribe(ModuleLoadCB cb)
        {
            Callbacks.push(cb);
        }

        void process()
        {
            register gfModuleInfo* info;

            asm {
                mr info, r30
            }

            int numCB = Callbacks.size();
            for (int i = 0; i < numCB; i++)
            {
                Callbacks[i](info);
            }
        }
    }

    void hookModule(gfModuleInfo* info)
    {
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
    void onModuleLoaded(gfModuleInfo* info)
    {
        hookModule(info);
    }

    // void hookLoadedModules()
    // {
    //     gfModuleManager* manager = gfModuleManager::getInstance();

    //     for (int i = 0; i < 16; i++)
    //     {
    //         gfModuleInfo* info = NULL;

    //         // is module loaded
    //         if (manager->m_moduleInfos[i].m_flags >> 4 & 1)
    //         {
    //             info = &manager->m_moduleInfos[i];
    //         }

    //         if (info != NULL)
    //         {
    //             hookModule(info);
    //         }
    //     }
    // }

    void syInit()
    {
        API = new (Heaps::Syringe) CoreApi();
        // Creates an event that's fired whenever a module is loaded
        API->syHookEx(0x80026db4, reinterpret_cast<void*>(ModuleLoadEvent::process), OPT_SAVE_REGS | OPT_ORIG_PRE);
        API->syHookEx(0x800272e0, reinterpret_cast<void*>(ModuleLoadEvent::process), OPT_SAVE_REGS | OPT_ORIG_PRE);

        // subscribe to onModuleLoaded event to handle applying hooks
        API->moduleLoadEventSubscribe(static_cast<ModuleLoadCB>(onModuleLoaded));
    }

    bool faLoadPlugin(FAEntryInfo* info, const char* folder)
    {
        char tmp[0x80];
        if (info->name[0] == 0)
            sprintf(tmp, "%s/%s", folder, info->shortname);
        else
            sprintf(tmp, "%s/%s", folder, info->name);

        // Syringe::Plugin* plg = new (Heaps::Syringe) Syringe::Plugin(tmp);
        Syringe::Plugin plg = Syringe::Plugin(tmp);

        if (!plg.loadPlugin(API))
        {
            OSReport("[Syringe] Failed to load plugin (%s)\n", tmp);
            return false;
        }

        // Plugins.push(plg);
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
