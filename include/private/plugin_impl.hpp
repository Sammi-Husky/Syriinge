#pragma once

#include "public/plugin.hpp"
#include "public/plugin_meta.hpp"
#include <sr/sr_common.h>

class gfModule;

namespace SyringeCore {
    class CoreApi;
}

class PluginImpl : public Plugin {
public:
    PluginImpl(const char* path, SyringeCore::CoreApi* api, s32 id);
    ~PluginImpl();

    bool load(HeapType overrideHeap = Heaps::Invalid);
    bool loadIntoHeap(void* heap);
    void unload();
    void execute();

    gfModule* getModule() { return module; }
    PluginMeta* getMetadata() { return metadata; }
    PluginImpl* getNext() const { return next; }
    void setNext(PluginImpl* plugin) { next = plugin; }

    virtual SyringeCore::Hook* addHook(const u32 address, const void* hook, int moduleId = -1);
    virtual SyringeCore::Hook* addHookEx(const u32 address, const void* function, int options, int moduleId = -1);
    virtual void addEventHandler(Event::EventType type, SyringeCore::EventHandlerFN func);
    virtual void clearEventHandlers();

private:
    PluginImpl* next;
    char path[126];
    s32 id;
    PluginMeta* metadata;
    gfModule* module;
    SyringeCore::CoreApi* core;
};
