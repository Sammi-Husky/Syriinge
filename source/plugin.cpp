#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <gf/gf_module.h>
#include <string.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"

namespace Syringe {
    typedef PluginMeta* (*PluginPrologFN)();
    Plugin::Plugin(const char* path, SyringeCore::CoreApi* core, s32 id)
        : module(NULL),
          metadata(NULL),
          enable(true),
          core(core),
          id(id)
    {
        strncpy(this->path, path, sizeof(this->path));
    }

    bool Plugin::load()
    {
        char buff[10];
        gfFileIOHandle handle;
        handle.read(this->path, Heaps::MenuInstance, 0);

        // Default to the Syringe heap
        u32 heapId = Heaps::Syringe;

        // If the metadata is present (e.g. this is a reload)
        // then use the heapId from the metadata
        if (this->metadata)
        {
            heapId = this->metadata->FLAGS.heap;
        }

        void* buffer = handle.getBuffer();

        if (!buffer)
        {
            // Failed to load module
            return false;
        }

        // Create the gfModule
        this->module = gfModule::create(
            gfHeapManager::getHeap(static_cast<HeapType>(heapId)),
            buffer,
            handle.getSize());

        // We can release the file handle now that the module has been created
        handle.release();

        // Free the buffer allocated by the handle
        free(buffer);

        // Get plugin metadata from prolog if this is the
        // first time we've loaded this plugin
        if (!this->metadata)
        {
            this->metadata = reinterpret_cast<PluginPrologFN>(this->module->header->prologOffset)();
        }

        // Check Syringe version compatibility
        if (this->metadata->VERSION != Version(SYRINGE_VERSION))
        {
            this->metadata->VERSION.toString(this->metadata->VERSION, buff);
            OSReport("[Syringe] Warning: Plugin %s was built for Syringe v%s, but current version is v%s. This may cause instability.\n",
                     this->metadata->NAME, buff, SYRINGE_VERSION);
        }

        OSReport("[Syringe] Loaded plugin (%s, v%s)\n", this->metadata->NAME, buff);

        return true;
    }
    void Plugin::execute()
    {
        // Call the plugin entrypoint
        this->metadata->entrypoint(this);
        OSReport("[Syringe] Executing plugin (%s)\n", this->metadata->NAME);
    }
    void Plugin::unload()
    {
        OSReport("[Syringe] Unloading plugin (%s)\n", this->metadata->NAME);

        // Restore original instructions for all hooks
        this->core->undoHooksByOwner(this->id);

        // Clear all event listeners associated with this plugin
        this->clearEventHandlers();

        // Clear hooks otherwise when reloading duplicates will be added
        this->core->removeHooksByOwner(this->id);

        if (this->module)
        {
            // Should we unlink before deleting the module?
            delete this->module;
        }

        // Make sure we set the module pointer to NULL
        this->module = NULL;
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
        // Subscribe to the event
        SyringeCore::EventDispatcher::subscribe(type, func, this->id);
    }
    void Plugin::clearEventHandlers()
    {
        SyringeCore::EventDispatcher::unsubscribe(this->id);
    }

    Plugin::~Plugin()
    {
        // Unload the plugin and free resources
        this->unload();

        // Set metadata to NULL
        this->metadata = NULL;
    }
}