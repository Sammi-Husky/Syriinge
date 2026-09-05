#include <OS/OSError.h>
#include <OS/OSLink.h>
#include <gf/gf_heap_manager.h>
#include <gf/gf_module.h>
#include <string.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"
#include "sy_core.hpp"

typedef PluginMeta* (*PluginPrologFN)();
Plugin::Plugin(const char* path, SyringeCore::CoreApi* core, s32 id)
    : next(NULL),
      module(NULL),
      metadata(NULL),
      enable(true),
      core(core),
      id(id)
{
    strncpy(this->path, path, sizeof(this->path));
    this->path[sizeof(this->path) - 1] = '\0';
}

bool Plugin::load(HeapType overrideHeap)
{
    // Default to the Syringe heap
    u32 heapId = Heaps::Syringe;

    // If the metadata is present (e.g. this is a reload)
    // then use the heapId from the metadata
    if (this->metadata)
    {
        heapId = this->metadata->FLAGS.heap;
    }

    // An explicit override takes precedence.
    if (overrideHeap != Heaps::Invalid)
    {
        heapId = overrideHeap;
    }

    return this->loadIntoHeap(gfHeapManager::getHeap(static_cast<HeapType>(heapId)));
}

bool Plugin::loadIntoHeap(void* heap)
{
    if (this->module != NULL)
    {
        // Already loaded for current lifecycle.
        return true;
    }

    char buff[10];
    gfFileIOHandle handle;
    handle.read(this->path, Heaps::MenuInstance, 0);

    void* buffer = handle.getBuffer();

    if (!buffer)
    {
        // Failed to load module
        return false;
    }

    // Create the gfModule in the requested heap
    this->module = gfModule::create(
        heap,
        buffer,
        handle.getSize());

    // We can release the file handle now that the module has been created
    handle.release();

    // Free the buffer allocated by the handle
    gfHeapManager::free(buffer);

    // Get plugin metadata from the plugin
    PluginMeta* metadata = reinterpret_cast<PluginPrologFN>(this->module->header->prologOffset)();

    // Delete the old metadata before copying the new one to prevent memory leak
    delete this->metadata;

    // copy metadata to plugin instance
    this->metadata = new (Heaps::Syringe) PluginMeta;
    *this->metadata = *metadata;

    // Check Syringe version compatibility
    this->metadata->VERSION.toString(this->metadata->VERSION, buff);
    OSReport("[Syringe] Loaded plugin (%s, v%s)\n", this->metadata->NAME, buff);
    if (this->metadata->VERSION != Version(SYRINGE_VERSION))
    {
        OSReport("[Syringe] Warning: Plugin %s was built for Syringe v%s, but current version is v%s. This may cause instability.\n",
                 this->metadata->NAME, buff, SYRINGE_VERSION);
    }

    return true;
}
void Plugin::execute()
{
    if (this->metadata == NULL)
        return;

    OSReport("[Syringe] Executing plugin (%s)\n", this->metadata->NAME);

    // Call the plugin entrypoint
    this->metadata->entrypoint(this);
}
void Plugin::unload()
{
    if (this->metadata != NULL)
    {
        OSReport("[Syringe] Unloading plugin (%s)\n", this->metadata->NAME);
    }

    // Clear hooks otherwise when reloading duplicates will be added
    this->core->removeHooksByOwner(this->id);

    // Clear all event listeners associated with this plugin
    this->clearEventHandlers();

    // Call module epilog and clean up
    if (this->module)
    {
        // Call the module epilog before unlinking and deleting it
        reinterpret_cast<void (*)(void)>(this->module->header->epilogOffset)();

        // Unlink the module before deleting it
        OSUnlink(this->module->header);

        // Delete the module to free memory
        delete this->module;

        // Make sure we set the module pointer to NULL
        this->module = NULL;
    }

    // Ensure we clear the entrypoint to prevent accidental use-after-free
    if (this->metadata != NULL)
    {
        this->metadata->entrypoint = NULL;
    }
}

SyringeCore::Hook* Plugin::addHook(const u32 address, const void* function, int moduleId)
{
    SyringeCore::Hook* hook = core->syHook(address, function, this->id, moduleId);
    return hook;
}
SyringeCore::Hook* Plugin::addHookEx(const u32 address, const void* function, int options, int moduleId)
{
    SyringeCore::Hook* hook = core->syHookEx(address, function, options, this->id, moduleId);
    return hook;
}
void Plugin::addEventHandler(Event::EventType type, SyringeCore::EventHandlerFN func)
{
    core->EventManager.subscribe(type, func, this->id);
}
void Plugin::clearEventHandlers()
{
    core->EventManager.unsubscribe(this->id);
}

Plugin::~Plugin()
{
    // Unload the plugin and free resources
    this->unload();

    // Free metadata
    delete this->metadata;

    // Set metadata to NULL
    this->metadata = NULL;
}
