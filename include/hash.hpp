#pragma once

#include "plugin_meta.hpp"

#define SY_FNV_OFFSET_BASIS 0x811c9dc5u
#define SY_FNV_PRIME        0x01000193u

static inline u8 syToLower(u8 c)
{
    if (c >= 'A' && c <= 'Z')
        return (u8)(c + ('a' - 'A'));
    return c;
}

static inline u32 syHash(const char* str)
{
    u32 hash = SY_FNV_OFFSET_BASIS;
    while (*str)
    {
        hash ^= (u32)syToLower((u8)*str);
        hash *= SY_FNV_PRIME;
        str++;
    }
    return hash;
}

#define SY_BOOT_HASH            (syHash("BOOT"))
#define SY_SCMEMORYCHANGE_HASH  (syHash("scMemoryChange"))
#define SY_HASH(str)            (syHash(str))
#define SY_SCENE(name)          { SY_HASH(name), TRIGGER_SCENE, TRIGGER_LOAD, HEAP_METADATA, 0 }
#define SY_MODULE(file, action, heap) { SY_HASH(file), TRIGGER_MODULE, (action), (heap), 0 }
