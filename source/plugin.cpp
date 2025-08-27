#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <gf/gf_module.h>
#include <string.h>

#include "coreapi.hpp"
#include "hook.hpp"
#include "plugin.hpp"

namespace Syringe {
    typedef PluginMeta* (*PluginPrologFN)();
    Plugin::Plugin(const char* path, SyringeCore::CoreApi* core)
        : module(NULL),
          metadata(NULL),
          enable(true),
          core(core)
    {
        strncpy(this->path, path, sizeof(this->path));
    }

    gfModule* Plugin::loadPlugin()
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

        // Create the gfModule
        this->module = gfModule::create(
            gfHeapManager::getHeap(static_cast<HeapType>(heapId)),
            handle.getBuffer(),
            handle.getSize());

        // Free the buffer since we no longer need it
        free(handle.getBuffer());

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

        // Call the plugin entrypoint if TIMING_BOOT is set
        if (this->metadata->FLAGS.timing & TIMING_BOOT)
        {
            this->metadata->entrypoint(this);
            this->metadata->VERSION.toString(this->metadata->VERSION, buff);
            OSReport("[Syringe] Loaded plugin (%s, v%s)\n", this->metadata->NAME, buff);
        }
        else
        {
            // This is kind of dumb because we just loaded the plugin and are unloading it immediately
            // but if the plugin flags dictate a specific load type, we need to respect that. Currently
            // there is no way to get plugin flags without loading the plugin first to request them
            this->unloadPlugin();
        }

        return module;
    }

    SyringeCore::Hook* Plugin::addHook(const u32 address, const void* function, int moduleId)
    {
        SyringeCore::Hook* hook = core->syHook(address, function, false, moduleId);
        this->hooks.push(hook);
        return hook;
    }
    SyringeCore::Hook* Plugin::addHookEx(const u32 address, const void* function, int options, int moduleId)
    {
        SyringeCore::Hook* hook = core->syHookEx(address, function, options, false, moduleId);
        this->hooks.push(hook);
        return hook;
    }

    void Plugin::unloadPlugin()
    {
        OSReport("[Syringe] Unloading plugin (%s)\n", this->metadata->NAME);

        // Restore original instructions for all hooks
        for (int i = 0; i < this->hooks.size(); i++)
        {
            this->hooks[i]->undo();
        }

        // Clear hooks otherwise when reloading duplicates will be added
        this->hooks.clear();

        // Mark hook as disabled
        this->enable = false;

        // Should we unlink before deleting the module?
        delete this->module;
    }

    Plugin::~Plugin()
    {
        // Unload the plugin and free resources
        this->unloadPlugin();

        // Set metadata to NULL
        this->metadata = NULL;
    }
}