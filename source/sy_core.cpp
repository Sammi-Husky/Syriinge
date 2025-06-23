#include <FA.h>
#include <OS/OSCache.h>
#include <OS/OSError.h>
#include <gf/gf_module.h>
#include <stdio.h>
#include <vector.h>

#include "plugin.h"
#include "sy_core.h"
#include "sy_utils.h"

namespace SyringeCore {
    CoreApi* API = NULL;
    Vector<Hook*> Injections;
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
        API->syInlineHook(0x80026db4, reinterpret_cast<void*>(ModuleLoadEvent::process));
        API->syInlineHook(0x800272e0, reinterpret_cast<void*>(ModuleLoadEvent::process));

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

void CoreApi::syCustomHook(const u32 address, const void* replacement, int options, int moduleId)
{
    Hook* hook = new (Heaps::Syringe) Hook(address,
                                           reinterpret_cast<u32>(replacement),
                                           moduleId,
                                           options);

    if (hook->type == HOOK_STATIC)
    {
        hook->apply(address);
        OSReport("[Syringe] Patching %8x -> %8x\n", address, (u32)replacement);
    }

    SyringeCore::Injections.push(hook);
}
void CoreApi::syInlineHook(const u32 address, const void* replacement)
{
    CoreApi::syInlineHookRel(address, replacement, -1);
}
void CoreApi::syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
{
    Hook* hook = new (Heaps::Syringe) Hook(offset,
                                           reinterpret_cast<u32>(replacement),
                                           moduleId,
                                           OPT_SAVE_REGS | OPT_ORIG_PRE);

    if (hook->type == HOOK_STATIC)
    {
        hook->apply(offset);
        OSReport("[Syringe] Patching %8x -> %8x\n", offset, (u32)replacement);
    }

    SyringeCore::Injections.push(hook);
}
void CoreApi::sySimpleHook(const u32 address, const void* replacement)
{
    CoreApi::syReplaceFunc(address, replacement, NULL);
}
void CoreApi::sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
{
    CoreApi::syReplaceFuncRel(offset, replacement, NULL, moduleId);
}
void CoreApi::syReplaceFunc(const u32 address, const void* replacement, void** original)
{
    CoreApi::syReplaceFuncRel(address, replacement, original, -1);
}
void CoreApi::syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
{
    Hook* hook = new (Heaps::Syringe) Hook(offset,
                                           reinterpret_cast<u32>(replacement),
                                           moduleId,
                                           OPT_DIRECT);

    if (hook->type == HOOK_STATIC)
    {
        OSReport("[Syringe] Patching %8x -> %8x\n", offset, (u32)replacement);
        hook->apply(offset);
    }

    SyringeCore::Injections.push(hook);
    *original = &hook->trampoline;
}
void CoreApi::moduleLoadEventSubscribe(SyringeCore::ModuleLoadCB cb)
{
    SyringeCore::ModuleLoadEvent::Subscribe(cb);
}

Hook::Hook(u32 source, u32 dest, s8 moduleId, int opts) : trampoline(Trampoline(0x60000000, 0)),
                                                          tgtAddr(source),
                                                          newAddr(dest),
                                                          options((HookOptions)opts),
                                                          moduleId(moduleId),
                                                          originalInstr(0x60000000)
{
    type = moduleId == -1 ? HOOK_STATIC : HOOK_RELATIVE; // determine hook type based on moduleId

    for (int i = 0; i < sizeof(instructions) / sizeof(u32); i++)
    {
        instructions[i] = 0x60000000; // initialize all instructions to NOP
    }
}
void Hook::setInstructions(u32 targetAddr, HookOptions opts)
{
    // original instruction or nop (ORIG_INSTR_PRE)
    if (opts & OPT_ORIG_PRE)
        instructions[0] = originalInstr;

    if (opts & OPT_SAVE_REGS)
    {
        instructions[1] = 0x9421FF70; // stwu r1, -0x90(r1)
        instructions[2] = 0x90010008; // stw r0, 0x8(r1)
        instructions[3] = 0xBC61000C; // stmw r3, 0xC(r1)
        instructions[4] = 0x7C0802A6; // mflr r0
        instructions[5] = 0x90010094; // stw r0, 0x94(r1)
        instructions[6] = SyringeUtils::EncodeBranch((u32)&instructions[6], newAddr, true);
        instructions[7] = 0x80010094;  // lwz r0, 0x94(r1)
        instructions[8] = 0x7C0803A6;  // mtlr r0
        instructions[9] = 0x80010008;  // lwz r0, 0x8(r1)
        instructions[10] = 0xB861000C; // lmw r3, 0xC(r1)
        instructions[11] = 0x38210090; // addi r1, r1, 0x90
    }
    else
    {
        instructions[1] = 0x9421FFF0; // stwu r1, -0x10(r1)
        instructions[2] = 0x7C0802A6; // mflr r0
        instructions[3] = 0x90010014; // stw r0, 0x14(r1)
        instructions[4] = SyringeUtils::EncodeBranch((u32)&instructions[6], newAddr, true);
        instructions[5] = 0x80010014; // lwz r0, 0x14(r1)
        instructions[6] = 0x7C0803A6; // mtlr r0
        instructions[7] = 0x38210010; // addi r1, r1, 0x10
    }

    // original instruction or nop (ORIG_INSTR_POST)
    if (opts & OPT_ORIG_POST)
        instructions[12] = originalInstr;

    // By default, we branch to the original function
    instructions[13] = SyringeUtils::EncodeBranch((u32)&instructions[13], targetAddr + 4);

    // If OPT_NO_RETURN is set, we branch to the link register instead
    if (opts & OPT_NO_RETURN)
    {
        instructions[13] = 0x4E800020; // blr
    }
}
void Hook::apply(u32 address)
{
    // get original instruction
    originalInstr = *(u32*)address;

    // Update the trampoline
    trampoline.originalInstr = originalInstr;
    trampoline.branch = SyringeUtils::EncodeBranch((u32)&trampoline.branch, address + 4);

    // Set the instructions for the hook
    this->setInstructions(address, options);

    if (options & OPT_DIRECT)
    {
        // If OPT_DIRECT is set, we directly branch to the new address
        *(u32*)address = SyringeUtils::EncodeBranch(address, newAddr);
    }
    else
    {
        // Otherwise, we patch the target address with a branch to our hook instructions
        *(u32*)address = SyringeUtils::EncodeBranch(address, (u32)&instructions[0]);
    }

    // invalidate instruction cache for the target address
    ICInvalidateRange((void*)address, 0x04);
}
Trampoline::Trampoline(u32 originalInstr, u32 retAddr) : originalInstr(originalInstr)
{
    branch = SyringeUtils::EncodeBranch((u32)&branch, retAddr);
}