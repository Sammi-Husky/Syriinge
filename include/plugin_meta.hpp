#pragma once

#include "version.hpp"

class Plugin;

enum LoadType {
    LOAD_PERSIST = 0, // Persists plugin across scenes
    LOAD_UNLOAD = 1   // Unloads plugin when the scene changes
};

typedef union {
    struct {
        u32 heap : 8;       // Heap to allocate the plugin in
        u32 loading : 1;    // Controls whether the plugin is loaded persistently or unloaded on scene change
        u32 _reserved : 23; // Reserved for future use
    };
    u32 value; // Combined flags
} PluginFlags;

const int MAX_LOAD_TRIGGERS = 10;

enum TriggerKind {
    TRIGGER_SCENE = 0, // Keyed by scene name (gfScene::m_sceneName)
    TRIGGER_MODULE = 1 // Keyed by module filename (e.g. "ft_mario.rel")
};

enum TriggerAction {
    TRIGGER_LOAD = 0,
    TRIGGER_UNLOAD = 1
};

enum TriggerHeap {
    HEAP_METADATA = 0, // Use the plugin metadata heap (PluginFlags.heap)
    HEAP_PIGGYBACK = 1 // Use the triggering module's own heap
};

struct PluginTrigger {
    u32 key;    // FNV-1a hash of the scene name / module filename
    u8 kind;    // TriggerKind
    u8 action;  // TriggerAction
    u8 heapSrc; // TriggerHeap (module loads only)
    u8 _pad;    // alignment / reserved
};

struct PluginMeta {
    char NAME[20];
    char AUTHOR[20];
    Version VERSION;
    Version SY_VERSION;
    void (*entrypoint)(Plugin* plg);
    PluginFlags FLAGS;
    PluginTrigger LOAD_TRIGGERS[MAX_LOAD_TRIGGERS];
};
