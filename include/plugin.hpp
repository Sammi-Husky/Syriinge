#pragma once

#include "eventDispatcher.hpp"
#include "events.hpp"
#include "version.hpp"
#include <sr/sr_common.h>

class gfModule;
namespace SyringeCore {
    class CoreApi;
    class Hook;
}
struct PluginMeta;

class Plugin {
public:
    Plugin(const char* path, SyringeCore::CoreApi* api, s32 id);
    ~Plugin();

    /**
     * @brief Loads the plugin and returns a pointer to the module.
     *
     * @param overrideHeap Optional heap to allocate the plugin module into. When
     * left as Heaps::Invalid the metadata heap (PluginFlags.heap) is used. Module
     * piggyback loads should instead call loadIntoHeap() with the triggering
     * module's heap pointer so the plugin shares that module's heap/lifetime.
     */
    virtual bool load(HeapType overrideHeap = Heaps::Invalid);
    /**
     * @brief Loads the plugin into a specific, already-resolved heap.
     *
     * Used for module "piggyback" loads where the target heap is the heap the
     * game loaded the triggering module into (a raw heap pointer, not an ID).
     *
     * @param heap Resolved heap pointer to allocate the plugin module into.
     */
    virtual bool loadIntoHeap(void* heap);
    /**
     * @brief Unloads the plugin and restores original instructions for all hooks.
     */
    virtual void unload();
    /**
     * @brief Executes the plugin's main functionality.
     */
    virtual void execute();
    /**
     * @brief Checks if the plugin is enabled.
     * @return True if the plugin is enabled, false otherwise.
     */
    virtual bool isEnabled() { return enable; }
    /**
     * @brief Gets the core API instance.
     * @warning The hooking functions contained in the core API are much lower level than the ones
     * provided by the Plugin class. Only use this if you need more advanced functionality not provided
     * by the Plugin instance
     * @return Pointer to the core API instance.
     */
    virtual SyringeCore::CoreApi* getCoreApi() { return core; }
    /**
     * @brief Gets the loaded module.
     * @return Pointer to the loaded module.
     */
    virtual gfModule* getModule() { return module; }
    /**
     * @brief Gets the plugin metadata.
     * @return Pointer to the plugin metadata.
     */
    virtual PluginMeta* getMetadata() { return metadata; }
    /**
     * @brief Injects a hook at the target address and registers it with the plugin
     * @note This hook will automatically return execution to the original function but will not save registers or execute the overwritten instruction
     *
     *
     * @param address address or offset to inject our hook at
     * @param function pointer to the function to run
     * @param moduleId (optional) ID of the target module, -1 for static hooks
     * @return Pointer to the created hook object
     */
    virtual SyringeCore::Hook* addHook(const u32 address, const void* hook, int moduleId = -1);
    /**
     * @brief Injects a hook at the target address with additional options and registers it with the plugin.
     *
     * @param address address or offset to inject our hook at
     * @param function pointer to the function to inject
     * @param options options for the hook
     * @param moduleId (optional) ID of the target module
     * @returns pointer to the created hook
     */
    virtual SyringeCore::Hook* addHookEx(const u32 address, const void* function, int options, int moduleId = -1);

    /**
     * @brief Adds an event handler for the specified event type.
     *
     * @param type The event type to subscribe to.
     * @param func The function to call when the event is triggered.
     */
    virtual void addEventHandler(Event::EventType type, SyringeCore::EventHandlerFN func);
    /**
     * @brief Clears all event handlers for the plugin.
     */
    virtual void clearEventHandlers();

private:
    char path[126];             // Path to the plugin file
    s32 id;                     // Unique ID for the plugin
    PluginMeta* metadata;       // Pointer to the plugin metadata
    gfModule* module;           // Pointer to the loaded module
    bool enable;                // Indicates if the plugin is enabled
    SyringeCore::CoreApi* core; // Pointer to the core API
};

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

/**
 * @brief Whether a trigger reacts to a scene change or a game module load/unload.
 */
enum TriggerKind {
    TRIGGER_SCENE = 0, // Keyed by scene name (gfScene::m_sceneName)
    TRIGGER_MODULE = 1 // Keyed by module filename (e.g. "ft_mario.rel")
};

/**
 * @brief What a trigger does when it fires.
 * @note TRIGGER_UNLOAD is only meaningful for TRIGGER_MODULE. Scenes are load-only;
 * scene-driven unloading is governed by the persistence model (PluginFlags.loading)
 * and the scMemoryChange pass.
 */
enum TriggerAction {
    TRIGGER_LOAD = 0,
    TRIGGER_UNLOAD = 1
};

/**
 * @brief Heap source for a module load trigger.
 */
enum TriggerHeap {
    HEAP_METADATA = 0, // Use the plugin metadata heap (PluginFlags.heap)
    HEAP_PIGGYBACK = 1 // Use the triggering module's own heap
};

/**
 * @brief A single load/unload trigger descriptor.
 *
 * Compact, allocation-free replacement for the old string array. The key is a
 * case-insensitive FNV-1a hash of either a scene name or a module filename
 * (see hash.hpp). 8 bytes each.
 */
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