#pragma once

#include "eventDispatcher.hpp"
#include "events.hpp"
#include "version.hpp"

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
     */
    virtual bool load();
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
     * @note Hooks injected via this function WILL automatically return execution to the original function.
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

enum LoadTiming {
    TIMING_BOOT = 1 << 0,        // Load the plugin when Syringe first loads
    TIMING_MAIN_MENU = 1 << 1,   // Load the plugin when the main menu is opened
    TIMING_CHAR_SELECT = 1 << 2, // Load the plugin when the character select screen is opened
    TIMING_MATCH = 1 << 3,       // Load the plugin when a match is started
    // TIMING_SUBSPACE = 1 << 4     // Load the plugin when entering subspace
};

enum LoadType {
    LOAD_PERSIST = 1 << 0, // Persists plugin across scenes
    LOAD_UNLOAD = 1 << 1   // Unloads plugin when the scene changes
};

typedef union {
    struct {
        u32 timing : 5;     // Controls when the plugin is loaded
        u32 loading : 2;    // Controls whether the plugin is loaded persistently or unloaded on scene change
        u32 heap : 8;       // Ignored if `timing` is set to `TIMING_BOOT`
        u32 _reserved : 17; // Reserved for future use
    };
    u32 value; // Combined flags
} PluginFlags;

struct PluginMeta {
    char NAME[20];
    char AUTHOR[20];
    Version VERSION;
    Version SY_VERSION;
    void (*entrypoint)(Plugin* plg);
    PluginFlags FLAGS;
};