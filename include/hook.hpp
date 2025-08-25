#pragma once

#include <types.h>

namespace SyringeCore {
    enum HookType {
        HOOK_STATIC,
        HOOK_RELATIVE
    };

    enum HookOptions {
        OPT_NONE = 0x0,      // no options
        OPT_SAVE_REGS = 0x1, // use safe hook
        OPT_ORIG_PRE = 0x2,  // run the original instruction before hook
        OPT_ORIG_POST = 0x4, // run the original instruction after the hook
        OPT_NO_RETURN = 0x8, // do not return to hooked function
        OPT_DIRECT = 0x10    // use direct branch instead of wrapping in a trampoline
    };

    struct Trampoline {
        Trampoline(u32 originalInstr, u32 retAddr);
        u32 originalInstr; // original instruction
        u32 branch;        // branch to original func code + 4
    };

    class Hook {
    public:
        HookType type;         // type of hook (static or relative)
        HookOptions options;   // hook options
        s8 moduleId;           // module ID this hook belongs to
        u32 tgtAddr;           // target address to hook
        u32 newAddr;           // address to branch to
        u32 instructions[14];  // hook instructions
        u32 originalInstr;     // original instruction at target address
        Trampoline trampoline; // trampoline to facilitate calling original function

        Hook(u32 source, u32 dest, s8 moduleId, int options);
        void apply(u32 address);

    private:
        void setInstructions(u32 targetAddr, HookOptions opts);
    };
}