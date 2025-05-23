#include <FA.h>
#include <OS/OSCache.h>
#include <OS/OSError.h>
#include <stdio.h>
#include <vector.h>

#include "plugin.h"
#include "sy_core.h"
#include "sy_utils.h"

namespace SyringeCore {
    CoreApi* API = NULL;
    Vector<InjectionAbs*> Injections;
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

    void doPatchesOnModule(gfModuleInfo* info)
    {
        gfModuleHeader* header = info->m_module->header;

        u32 textAddr = header->getTextSectionAddr();
        int numInjections = Injections.size();

        for (int i = 0; i < numInjections; i++)
        {
            InjectionAbs* inject = Injections[i];
            if (inject->moduleId != header->id)
            {
                continue;
            }

            u32 targetAddr = inject->tgtAddr;

            // if this is a module hook, add offset to .text addr
            if (targetAddr < 0x80000000)
            {
                targetAddr += textAddr;
            }

            // sets original instruction now that module has been loaded into memory.
            // This differs for replacements because only the trampline back
            // needs the original instruction
            if (inject->type == INJECT_TYPE_INLINE)
            {
                InlineHook* tmp = (InlineHook*)inject;
                tmp->originalInstr = *(u32*)targetAddr;

                u32 hookAddr = (u32)&tmp->originalInstr;
                *(u32*)targetAddr = SyringeUtils::EncodeBranch(targetAddr, hookAddr);
                OSReport("[Syringe] Patching %8x -> %8x\n", targetAddr, hookAddr);
                // encode hook with branch back to injection point
                u32 returnBranch = (u32)&tmp->instructions[11];
                tmp->instructions[11] = SyringeUtils::EncodeBranch(returnBranch, (targetAddr + 4));
            }
            else if (inject->type == INJECT_TYPE_REPLACE)
            {
                Hook* asHook = (Hook*)inject;

                // it's important we refresh this before
                // patching the target with the hook branch
                if (asHook->trampoline != NULL)
                {
                    u32 returnBranch = (u32)&asHook->trampoline->branch;
                    asHook->trampoline->originalInstr = *(u32*)targetAddr;
                    asHook->trampoline->branch = SyringeUtils::EncodeBranch(returnBranch, (targetAddr + 4));
                }

                u32 branchAddr = (u32)&asHook->branch;
                *(u32*)targetAddr = SyringeUtils::EncodeBranch(targetAddr, branchAddr);
                OSReport("[Syringe] Patching %8x -> %8x\n", targetAddr, branchAddr);
            }
            ICInvalidateRange((void*)targetAddr, 0x04);
        }
    }
    void onModuleLoaded(gfModuleInfo* info)
    {
        doPatchesOnModule(info);
    }
    void applyRelHooks()
    {
        gfModuleManager* manager = gfModuleManager::getInstance();

        for (int i = 0; i < 16; i++)
        {
            gfModuleInfo* info = NULL;

            // is module loaded
            if (manager->m_moduleInfos[i].m_flags >> 4 & 1)
            {
                info = &manager->m_moduleInfos[i];
            }

            if (info != NULL)
            {
                doPatchesOnModule(info);
            }
        }
    }

    void syInit()
    {
        CoreApi* api = new (Heaps::Syringe) CoreApi();
        API = api;
        // Creates an event that's fired whenever a module is loaded
        api->syInlineHook(0x80026db4, reinterpret_cast<void*>(ModuleLoadEvent::process));
        api->syInlineHook(0x800272e0, reinterpret_cast<void*>(ModuleLoadEvent::process));

        // subscribe to onModuleLoaded event to handle applying hooks
        api->moduleLoadEventSubscribe(static_cast<ModuleLoadCB>(onModuleLoaded));
    }

    void _inlineHook(const u32 address, const void* replacement, int moduleId)
    {
        // set up our trampoline for calling original
        InlineHook* hook = new InlineHook();
        hook->type = INJECT_TYPE_INLINE;
        hook->moduleId = moduleId;
        hook->tgtAddr = address;

        // // no need to patch immediately if target is inside a rel
        if (moduleId == -1)
        {
            OSReport("[Syringe] Patching %8x -> %8x\n", address, (u32)replacement);
            hook->originalInstr = *(u32*)address;

            // patch target func with hook
            u32 hookAddr = (u32)&hook->originalInstr;
            *(u32*)address = SyringeUtils::EncodeBranch(address, hookAddr);
        }

        // encode hook with jump to our func
        u32 replAddr = reinterpret_cast<u32>(replacement);
        u32 replBranchAddr = (u32)&hook->instructions[5];
        hook->instructions[5] = SyringeUtils::EncodeBranch(replBranchAddr, replAddr, true);

        // encode hook with branch back to injection point
        u32 returnBranch = (u32)&hook->instructions[11];
        hook->instructions[11] = SyringeUtils::EncodeBranch(returnBranch, (address + 4));

        Injections.push(hook);

        ICInvalidateRange((void*)address, 0x04);
    }
    void _replaceFunc(const u32 address, const void* replacement, void** original, int moduleId)
    {
        Hook* hook = new Hook();
        hook->type = INJECT_TYPE_REPLACE;
        hook->moduleId = moduleId;
        hook->tgtAddr = address;

        if (original != NULL)
        {
            // encode our trampoline branch
            // back to original func
            Trampoline* tramp = new Trampoline();

            // only read immediately if patch is inside a rel
            if (moduleId == -1)
            {
                tramp->originalInstr = *(u32*)address;
            }

            u32 trampBranch = (u32)&tramp->branch;
            tramp->branch = SyringeUtils::EncodeBranch(trampBranch, address + 4);

            *original = tramp;
            hook->trampoline = tramp;
        }

        u32 replAddr = reinterpret_cast<u32>(replacement);
        u32 hookBranch = (u32)&hook->branch;
        hook->branch = SyringeUtils::EncodeBranch(hookBranch, replAddr);

        // no need to patch immediately if target is inside a rel
        if (moduleId == -1)
        {
            // patch target func with hook
            OSReport("[Syringe] Patching %8x -> %8x\n", address, (u32)replacement);
            *(u32*)address = SyringeUtils::EncodeBranch(address, hookBranch);
            ICInvalidateRange((void*)address, 0x04);
        }

        Injections.push(hook);
    }
    bool _faLoadPlugin(FAEntryInfo* info, const char* folder)
    {
        char tmp[0x80];
        if (info->name[0] == 0)
            sprintf(tmp, "%s/%s", folder, info->shortname);
        else
            sprintf(tmp, "%s/%s", folder, info->name);

        // Syringe::Plugin* plg = new (Heaps::Syringe) Syringe::Plugin(tmp);
        Syringe::Plugin plg = Syringe::Plugin(tmp);

        if (!plg.loadPlugin())
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
            if (_faLoadPlugin(&info, folder))
                count++;

            // Loop over and load the rest if there are more
            while (FAFsnext(&info) == 0)
            {
                if (_faLoadPlugin(&info, folder))
                    count++;
            }
        }
        return count;
    }
} // namespace SyringeCore

void CoreApi::syInlineHook(const u32 address, const void* replacement)
{
    SyringeCore::_inlineHook(address, replacement, -1);
}
void CoreApi::syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
{
    SyringeCore::_inlineHook(offset, replacement, moduleId);
}
void CoreApi::sySimpleHook(const u32 address, const void* replacement)
{
    SyringeCore::_replaceFunc(address, replacement, NULL, -1);
}
void CoreApi::sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
{
    SyringeCore::_replaceFunc(offset, replacement, NULL, moduleId);
}
void CoreApi::syReplaceFunc(const u32 address, const void* replacement, void** original)
{
    SyringeCore::_replaceFunc(address, replacement, original, -1);
}
void CoreApi::syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
{
    SyringeCore::_replaceFunc(offset, replacement, original, moduleId);
}
void CoreApi::moduleLoadEventSubscribe(SyringeCore::ModuleLoadCB cb)
{
    SyringeCore::ModuleLoadEvent::Subscribe(cb);
}