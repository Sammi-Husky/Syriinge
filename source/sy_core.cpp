#include "sy_core.h"
#include "sy_utils.h"
#include <OS/OSCache.h>
#include <OS/OSError.h>
#include <vector.h>

namespace SyringeCore {
    Vector<InjectionAbs*> Injections;

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

    void onModuleLoaded(gfModuleInfo* info)
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
                inject->originalInstr = *(u32*)targetAddr;

                u32 hookAddr = (u32)&inject->originalInstr;
                *(u32*)targetAddr = SyringeUtils::EncodeBranch(targetAddr, hookAddr);
                OSReport("[syCore] Hooked: %x -> %x\n", targetAddr, hookAddr);
            }
            else if (inject->type == INJECT_TYPE_REPLACE)
            {
                Hook* asHook = (Hook*)inject;

                // it's important we refresh this before
                // patching the target with the hook branch
                if (asHook->trampoline != NULL)
                {
                    asHook->trampoline->originalInstr = *(u32*)targetAddr;
                }

                u32 branchAddr = (u32)&asHook->branch;
                *(u32*)targetAddr = SyringeUtils::EncodeBranch(targetAddr, branchAddr);
                OSReport("[syCore] Hooked: %x -> %x\n", targetAddr, branchAddr);
            }
            ICInvalidateRange((void*)targetAddr, 0x04);
        }
    }

    void syInit()
    {
        // Reload hooks every time a module is loaded
        SyringeCore::syInlineHook(0x80026db4, reinterpret_cast<void*>(ModuleLoadEvent::process));
        SyringeCore::syInlineHook(0x800272e0, reinterpret_cast<void*>(ModuleLoadEvent::process));

        ModuleLoadEvent::Subscribe(static_cast<ModuleLoadCB>(onModuleLoaded));
    }

    void syInlineHook(const u32 address, const void* replacement, int moduleId)
    {
        // set up our trampoline for calling original
        InlineHook* hook = new InlineHook();
        hook->type = INJECT_TYPE_INLINE;
        hook->moduleId = moduleId;
        hook->tgtAddr = address;

        // no need to patch immediately if target is inside a rel
        if (moduleId == -1)
        {
            hook->originalInstr = *(u32*)address;

            // patch target func with hook
            u32 hookAddr = (u32)&hook->originalInstr;
            *(u32*)address = SyringeUtils::EncodeBranch(address, hookAddr);
        }

        // encode hook with jump to our func
        u32 replAddr = reinterpret_cast<u32>(replacement);
        u32 replBranchAddr = (u32)&hook->instructions[4];
        hook->instructions[4] = SyringeUtils::EncodeBranch(replBranchAddr, replAddr, true);

        // encode hook with branch back to injection point
        u32 returnBranch = (u32)&hook->instructions[9];
        hook->instructions[9] = SyringeUtils::EncodeBranch(returnBranch, (address + 4));

        Injections.push(hook);

        ICInvalidateRange((void*)address, 0x04);
    }

    void syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        syInlineHook(offset, replacement, moduleId);
    }

    void sySimpleHook(const u32 address, const void* replacement, int moduleId)
    {
        syReplaceFunc(address, replacement, NULL, moduleId);
    }

    void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        sySimpleHook(offset, replacement, moduleId);
    }

    void syReplaceFunc(const u32 address, const void* replacement, void** original, int moduleId)
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
            *(u32*)address = SyringeUtils::EncodeBranch(address, hookBranch);
            ICInvalidateRange((void*)address, 0x04);
        }

        Injections.push(hook);
    }

    void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        syReplaceFunc(offset, replacement, original, moduleId);
    }

} // namespace SyringeCore
