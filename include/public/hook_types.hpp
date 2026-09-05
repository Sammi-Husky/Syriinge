#pragma once

namespace SyringeCore {
    class Hook;

    enum HookOptions {
        OPT_NONE = 0x0,
        OPT_SAVE_REGS = 0x1,
        OPT_ORIG_PRE = 0x2,
        OPT_ORIG_POST = 0x4,
        OPT_NO_RETURN = 0x8,
        OPT_DIRECT = 0x10
    };
}
