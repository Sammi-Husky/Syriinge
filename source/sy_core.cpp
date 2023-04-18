#include "sy_core.h"
#include "utils.h"
#include <OS/OSCache.h>
#include <gf/gf_module.h>
#include <vector.h>

namespace SyringeCore {
    Vector<InjectionAbs*> Injections;

    void reloadModuleHooks()
    {
        register gfModuleInfo* info;

        asm {
            mr info, r30
        }

        char id = *(int*)info->m_buffer;

        int numInjections = Injections.size();
        for (int i = 0; i < numInjections; i++)
        {
            InjectionAbs* inject = Injections[i];
            if (inject->moduleId != id)
            {
                continue;
            }

            // if full func replacement we don't need original
            // instruction. Branch directly to replacemnt.
            u32 targetAddr = inject->tgtAddr;
            if (inject->originalInstr == -1)
            {
                Hook* asHook = (Hook*)inject;

                // it's important we refresh this before
                // patching the target with the hook branch
                if (asHook->trampoline != NULL)
                {
                    asHook->trampoline->originalInstr = *(u32*)targetAddr;
                }

                u32 branchAddr = (u32)&asHook->branch;
                *(u32*)targetAddr = utils::EncodeBranch(targetAddr, branchAddr);
            }
            else
            {
                // refresh original instruction now that
                // module has been loaded into memory
                inject->originalInstr = *(u32*)targetAddr;

                u32 hookAddr = (u32)&inject->originalInstr;
                *(u32*)targetAddr = utils::EncodeBranch(targetAddr, hookAddr);
            }
            ICInvalidateRange((void*)targetAddr, 0x04);
        }
    }

    void syInit()
    {
        // Reload hooks every time a module is loaded
        SyringeCore::syHookFunction(0x80026db4, reinterpret_cast<void*>(reloadModuleHooks));
        SyringeCore::syHookFunction(0x800272e0, reinterpret_cast<void*>(reloadModuleHooks));
    }
    void syHookFunction(const u32 address, const void* replacement, int moduleId)
    {
        // set up our trampoline for calling original
        InlineHook* hook = new InlineHook();
        hook->originalInstr = *(u32*)address;
        hook->moduleId = moduleId;
        hook->tgtAddr = address;

        // patch target func with hook
        u32 hookAddr = (u32)&hook->originalInstr;
        *(u32*)address = utils::EncodeBranch(address, hookAddr);

        // encode hook with jump to our func
        u32 replAddr = reinterpret_cast<u32>(replacement);
        u32 replBranchAddr = (u32)&hook->instructions[4];
        hook->instructions[4] = utils::EncodeBranch(replBranchAddr, replAddr, true);

        // encode hook with branch back to injection point
        u32 returnBranch = (u32)&hook->instructions[9];
        hook->instructions[9] = utils::EncodeBranch(returnBranch, (address + 4));

        Injections.push(hook);

        ICInvalidateRange((void*)address, 0x04);
    }
    void sySimpleHook(const u32 address, const void* replacement, int moduleId)
    {
        syReplaceFunction(address, replacement, NULL, moduleId);
    }
    void syReplaceFunction(const u32 address, const void* replacement, void** original, int moduleId)
    {
        Hook* hook = new Hook();
        hook->moduleId = moduleId;
        hook->tgtAddr = address;

        if (original != NULL)
        {
            // encode our trampoline branch
            // back to original func
            Trampoline* tramp = new Trampoline();
            tramp->originalInstr = *(u32*)address;

            u32 trampBranch = (u32)&tramp->branch;
            tramp->branch = utils::EncodeBranch(trampBranch, address + 4);

            *original = tramp;
            hook->trampoline = tramp;
        }

        u32 replAddr = reinterpret_cast<u32>(replacement);
        u32 hookBranch = (u32)&hook->branch;
        hook->branch = utils::EncodeBranch(hookBranch, replAddr);

        // patch target func with hook
        *(u32*)address = utils::EncodeBranch(address, hookBranch);

        Injections.push(hook);
        ICInvalidateRange((void*)address, 0x04);
    }
    void syReplaceFunction(const void* symbol, const void* replacement, void** original, int moduleId)
    {
        return syReplaceFunction((u32)symbol, replacement, original, moduleId);
    }

} // namespace SyringeCore
