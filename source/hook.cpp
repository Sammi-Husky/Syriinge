#include <OS/OSCache.h>

#include "hook.hpp"
#include "sy_utils.hpp"
#include <internal/mem.h>
namespace SyringeCore {
    // Payload template for hooks that need to save registers
    static const u32 safe_payload[] = {
        0x9421FF70, // stwu r1, -0x90(r1)
        0x90010008, // stw r0, 0x8(r1)
        0xBC61000C, // stmw r3, 0xC(r1)
        0x7C0802A6, // mflr r0
        0x90010094, // stw r0, 0x94(r1)
        0x48000001, // bl <address>
        0x80010094, // lwz r0, 0x94(r1)
        0x7C0803A6, // mtlr r0
        0x80010008, // lwz r0, 0x8(r1)
        0xB861000C, // lmw r3, 0xC(r1)
        0x38210090, // addi r1, r1, 0x90
    };

    // Payload template for hooks that don't need to save registers
    static const u32 simple_payload[] = {
        0x9421FFF0, // stwu r1, -0x10(r1)
        0x7C0802A6, // mflr r0
        0x90010014, // stw r0, 0x14(r1)
        0x48000001, // bl <address>
        0x80010014, // lwz r0, 0x14(r1)
        0x7C0803A6, // mtlr r0
        0x38210010, // addi r1, r1, 0x10
    };

    Hook::Hook(u32 source, u32 dest, s8 moduleId, int opts)
        : trampoline(Trampoline(0x60000000, 0)),
          tgtAddr(source),
          newAddr(dest),
          options((HookOptions)opts),
          moduleId(moduleId)
    {
        type = moduleId == -1 ? HOOK_STATIC : HOOK_RELATIVE; // determine hook type based on moduleId

        for (int i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++)
        {
            instructions[i] = 0x60000000; // initialize all instructions to NOP
        }
    }
    void Hook::setInstructions(u32 targetAddr, HookOptions opts)
    {
        if (opts & OPT_SAVE_REGS)
        {
            memcpy(&instructions, &safe_payload, sizeof(safe_payload));
            instructions[6] = SyringeUtils::EncodeBranch((u32)&instructions[6], newAddr, true);
        }
        else
        {
            memcpy(&instructions, &simple_payload, sizeof(simple_payload));
            instructions[4] = SyringeUtils::EncodeBranch((u32)&instructions[4], newAddr, true);
        }

        // original instruction or nop (ORIG_INSTR_PRE)
        if (opts & OPT_ORIG_PRE)
            instructions[0] = originalInstr;

        // original instruction or nop (ORIG_INSTR_POST)
        if (opts & OPT_ORIG_POST)
            instructions[12] = originalInstr;

        // If OPT_NO_RETURN is set, we branch to the link register
        if (opts & OPT_NO_RETURN)
        {
            instructions[13] = 0x4E800020; // blr
        }

        // By default, we branch to the original function
        instructions[13] = SyringeUtils::EncodeBranch((u32)&instructions[13], targetAddr + 4);
    }
    void Hook::apply(u32 address)
    {
        // get original instruction
        originalInstr = *(u32*)address;

        // Update the trampoline
        trampoline.originalInstr = originalInstr;
        trampoline.branch = SyringeUtils::EncodeBranch((u32)&trampoline.branch, address + 4);

        if (options & OPT_DIRECT)
        {
            // If OPT_DIRECT is set, we directly branch to the new address
            *(u32*)address = SyringeUtils::EncodeBranch(address, newAddr);
        }
        else
        {
            // Set the instructions for the hook
            this->setInstructions(address, options);

            // patch the target address with a branch to our hook instructions
            *(u32*)address = SyringeUtils::EncodeBranch(address, (u32)&instructions[0]);
        }

        // invalidate instruction cache for the target address
        ICInvalidateRange((void*)address, 0x04);
    }

    Trampoline::Trampoline(u32 originalInstr, u32 retAddr) : originalInstr(originalInstr)
    {
        branch = SyringeUtils::EncodeBranch((u32)&branch, retAddr);
    }
}