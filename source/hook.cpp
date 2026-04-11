#include <OS/OSCache.h>
#include <memory.h>

#include "hook.hpp"
#include "sy_utils.hpp"
#include <internal/mem.h>
namespace SyringeCore {
    // Payload template for hooks that need to save registers
    static const u32 safe_payload[] = {
        0x60000000, // nop (placeholder for original instruction if OPT_ORIG_PRE is set)
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
        0x60000000, // nop (placeholder for original instruction if OPT_ORIG_POST is set)
        0x60000000, // nop (placeholder for final branch instruction)
    };

    // Payload template for hooks that don't need to save registers
    static const u32 simple_payload[] = {
        0x60000000, // nop (placeholder for original instruction if OPT_ORIG_PRE is set)
        0x9421FFF0, // stwu r1, -0x10(r1)
        0x7C0802A6, // mflr r0
        0x90010014, // stw r0, 0x14(r1)
        0x48000001, // bl <address>
        0x80010014, // lwz r0, 0x14(r1)
        0x7C0803A6, // mtlr r0
        0x38210010, // addi r1, r1, 0x10
        0x60000000, // nop (placeholder for original instruction if OPT_ORIG_POST is set)
        0x60000000, // nop (placeholder for final branch instruction)
    };

    // constants for calculating payload sizes and branch instruction indices based on options
    enum {
        SAFE_PAYLOAD_WORDS = sizeof(safe_payload) / sizeof(safe_payload[0]),
        SIMPLE_PAYLOAD_WORDS = sizeof(simple_payload) / sizeof(simple_payload[0]),
        SAFE_BRANCH_IDX = 6,
        SIMPLE_BRANCH_IDX = 4
    };

    static u8 calcPayloadWords(HookOptions opts)
    {
        if (opts & OPT_DIRECT)
            return 0;

        else if (opts & OPT_SAVE_REGS)
            return SAFE_PAYLOAD_WORDS;
        else
            return SIMPLE_PAYLOAD_WORDS;
    }

    Hook::Hook(u32 source, u32 dest, u32 moduleId, int opts, s32 owner)
        : trampoline(Trampoline(0x60000000, 0)),
          tgtAddr(source),
          newAddr(dest),
          options((HookOptions)opts),
          moduleId(moduleId),
          owner(owner),
          payload(NULL),
          installedAt(NULL)
    {
        const u8 words = calcPayloadWords(options);
        if (words != 0)
        {
            payload = new (Heaps::Syringe) u32[words];
            memcpy(payload,
                   (options & OPT_SAVE_REGS) ? safe_payload : simple_payload,
                   words * sizeof(u32));
        }
    }

    Hook::~Hook()
    {
        if (payload != NULL)
        {
            delete[] payload;
            payload = NULL;
        }
    }

    /// @brief Updates the hook payload with the correct branch and original instruction (if needed)
    /// @param targetAddr The address we are hooking
    void Hook::setInstructions(u32 targetAddr)
    {
        // If OPT_DIRECT is set, we don't use a payload
        if (payload == NULL)
            return;

        const bool saveRegs = (options & OPT_SAVE_REGS) != 0;
        const u8 branchIdx = saveRegs ? SAFE_BRANCH_IDX : SIMPLE_BRANCH_IDX;
        const u8 postIdx = saveRegs ? (SAFE_PAYLOAD_WORDS - 2) : (SIMPLE_PAYLOAD_WORDS - 2);

        if (options & OPT_ORIG_PRE)
            payload[0] = originalInstr;

        payload[branchIdx] = SyringeUtils::EncodeBranch((u32)&payload[branchIdx], newAddr);

        // original instruction (ORIG_INSTR_POST)
        if (options & OPT_ORIG_POST)
            payload[postIdx] = originalInstr;

        // If OPT_NO_RETURN is set, we branch to the link register
        // Otherwise, we branch to the original function + 4
        if (options & OPT_NO_RETURN)
        {
            payload[postIdx + 1] = 0x4E800020; // blr
        }
        else
        {
            payload[postIdx + 1] = SyringeUtils::EncodeBranch((u32)&payload[postIdx + 1], targetAddr + 4);
        }
    }

    /// @brief Applies the hook by writing the hook branch to the target address and setting up the payload if needed
    /// @param address Absolute address of the instruction to hook
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
            this->setInstructions(address);

            // patch the target address with a branch to our hook instructions
            *(u32*)address = SyringeUtils::EncodeBranch(address, (u32)&payload[0]);

            // invalidate instruction cache for the payload body
            ICInvalidateRange((void*)payload, calcPayloadWords(options) * sizeof(u32));
        }

        // Update the installedAt address
        installedAt = address;

        // invalidate instruction cache for the target address
        ICInvalidateRange((void*)address, 0x04);
    }

    /// @brief Undoes the hook by restoring the original instruction at the target address
    void Hook::undo()
    {
        // restore the original instruction at the target address
        *(u32*)installedAt = originalInstr;

        // invalidate instruction cache for the target address
        ICInvalidateRange((void*)installedAt, 0x04);

        // Clear variables to reflect that the hook is no longer active
        installedAt = NULL;
        originalInstr = NULL;
    }

    Trampoline::Trampoline(u32 originalInstr, u32 retAddr) : originalInstr(originalInstr), branch(0)
    {
        branch = SyringeUtils::EncodeBranch((u32)&branch, retAddr);
    }
}