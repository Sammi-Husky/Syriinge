#pragma once

#include <types.h>

/**
 * @file hash.hpp
 * @brief Case-insensitive FNV-1a hashing for plugin triggers.
 *
 * Both scene names and module filenames are reduced to a full 32-bit hash so
 * that the core can compare integers instead of storing/comparing strings.
 * This removes the per-string heap allocations that the old LOAD_TIMINGS
 * design required and keeps plugin metadata compact.
 *
 * The hash is case-insensitive to preserve the behavior of the previous
 * stricmp-based scene matching.
 *
 * NOTE: MWCC targets C++98, so there is no constexpr. Plugin metadata is
 * already constructed at runtime inside the plugin prolog (e.g. it calls
 * Version("1.0.0")), so hashing string literals via an inline function inside
 * the aggregate initializer is fine; the cost is paid once at load time.
 */

// FNV-1a 32-bit parameters
#define SY_FNV_OFFSET_BASIS 0x811c9dc5u
#define SY_FNV_PRIME        0x01000193u

/**
 * @brief Lowercases a single ASCII byte (A-Z -> a-z).
 */
static inline u8 syToLower(u8 c)
{
    if (c >= 'A' && c <= 'Z')
        return (u8)(c + ('a' - 'A'));
    return c;
}

/**
 * @brief Case-insensitive FNV-1a hash of a NUL-terminated string.
 *
 * @param str String to hash (must not be NULL).
 * @return 32-bit hash.
 */
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

// Well-known scene name hashes used by the core's evaluation paths.
// Kept as macros invoking syHash so the algorithm has a single definition.
#define SY_BOOT_HASH            (syHash("BOOT"))
#define SY_SCMEMORYCHANGE_HASH  (syHash("scMemoryChange"))

/**
 * @brief Compile-time-style helper for authoring trigger keys.
 */
#define SY_HASH(str) (syHash(str))

/**
 * @brief Declare a scene trigger. Scenes are load-only and always use the
 * heap specified in the plugin metadata (PluginFlags.heap).
 *
 * @param name Scene name (e.g. "scSelctCharacter"). "BOOT" loads at startup.
 */
#define SY_SCENE(name) \
    { SY_HASH(name), TRIGGER_SCENE, TRIGGER_LOAD, HEAP_METADATA, 0 }

/**
 * @brief Declare a module trigger.
 *
 * @param file   Module filename (e.g. "ft_mario.rel").
 * @param action TRIGGER_LOAD or TRIGGER_UNLOAD.
 * @param heap   HEAP_PIGGYBACK (load into the module's own heap) or
 *               HEAP_METADATA (use the plugin metadata heap). Only relevant
 *               for TRIGGER_LOAD.
 */
#define SY_MODULE(file, action, heap) \
    { SY_HASH(file), TRIGGER_MODULE, (action), (heap), 0 }
