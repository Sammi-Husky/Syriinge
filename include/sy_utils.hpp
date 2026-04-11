#pragma once

#include <types.h>

// Compile-time logging gate.
// Define LOGGING to enable log output; omit to strip all log callsites
// and their string literals from the binary, reducing code size.
#ifdef LOGGING
#include <OS/OSError.h>
#include <stdio.h>
inline void SY_LOG_IMPL(const char* fmt, ...)
{
    char buf[256];

    // Manually inject the prefix into the buffer so one OSReport call emits
    // the fully formatted message.
    char* p = buf;
    const char* prefix = "[Syringe] ";
    while (*prefix)
        *p++ = *prefix++;

    va_list args;
    va_start(args, fmt);
    p += vsprintf(p, fmt, args);
    va_end(args);

    OSReport("%s", buf);
}

#define SY_LOG(...) SY_LOG_IMPL(__VA_ARGS__)
#else
#define SY_LOG(...) ((void)0)
#endif

namespace SyringeUtils {
    inline u32 EncodeBranch(u32 start, u32 dest, bool linked)
    {
        u32 offset;
        if (start > dest)
        {
            offset = dest - start;
        }
        else
        {
            offset = -(start - dest);
        }
        u32 instr = 0x48000000 | offset & 0x3FFFFFF;
        return linked ? instr + 1 : instr;
    }
    inline u32 EncodeBranch(u32 start, u32 dest)
    {
        return EncodeBranch(start, dest, false);
    }
} // namespace SyringeUtils
