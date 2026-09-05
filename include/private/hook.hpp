#pragma once

#include "public/hook_types.hpp"
#include <types.h>

namespace SyringeCore {
    enum HookType {
        HOOK_STATIC,
        HOOK_RELATIVE
    };

    struct Trampoline {
        Trampoline(u32 originalInstr, u32 retAddr);
        u32 originalInstr;
        u32 branch;
    };

    class Hook {
    private:
        Hook* next;
        s32 owner;
        HookOptions options;
        u32 moduleId;
        u32 tgtAddr;
        u32 newAddr;
        u32 instructions[14];
        Trampoline trampoline;
        u32 installedAt;
        u32 originalInstr;
    public:
        Hook(u32 source, u32 dest, u32 moduleId, int options, s32 owner);
        HookType getType() const { return moduleId == -1 ? HOOK_STATIC : HOOK_RELATIVE; }
        HookOptions getOptions() const { return options; }
        u32 getModuleId() const { return moduleId; }
        u32 getTarget() const { return tgtAddr; }
        u32 getDestination() const { return newAddr; }
        u32 getInstalledAt() const { return installedAt; }
        void getTrampoline(void** func) { *func = &trampoline; }
        void* getPayloadAddr() { return &instructions; }
        u32 getOriginalInstr() const { return originalInstr; }
        s32 getOwner() const { return owner; }
        Hook* getNext() const { return next; }
        void setNext(Hook* hook) { next = hook; }
        void apply(u32 address);
        void undo();

    private:
        void setInstructions(u32 targetAddr, HookOptions opts);
    };
}
