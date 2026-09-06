#pragma once

#include "version.hpp"

class Plugin;

enum LoadType {
    LOAD_PERSIST = 0,
    LOAD_UNLOAD = 1
};

typedef union {
    struct {
        u32 heap : 8;
        u32 loading : 1;
        u32 _reserved : 23;
    };
    u32 value;
} PluginFlags;

const int MAX_LOAD_TRIGGERS = 10;

enum TriggerKind {
    TRIGGER_SCENE = 0,
    TRIGGER_MODULE = 1
};

enum TriggerAction {
    TRIGGER_LOAD = 0,
    TRIGGER_UNLOAD = 1
};

enum TriggerHeap {
    HEAP_METADATA = 0,
    HEAP_PIGGYBACK = 1
};

struct PluginTrigger {
    u32 key;
    u8 kind;
    u8 action;
    u8 heapSrc;
    u8 _pad;
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
