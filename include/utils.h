#pragma once

#include <types.h>

namespace utils {
    u32 EncodeBranch(u32 start, u32 dest, bool linked);
    u32 EncodeBranch(u32 start, u32 dest);
} // namespace utils
