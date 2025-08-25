#include <OS/OSError.h>
#include <gf/gf_heap_manager.h>
#include <gf/gf_module.h>
#include <string.h>

#include "plugin.hpp"

namespace Syringe {
    typedef PluginMeta* (*PluginPrologFN)(SyringeCore::CoreApi*);
    Plugin::Plugin(const char* path)
    {
        strncpy(this->path, path, sizeof(this->path));
    }

    gfModule* Plugin::loadPlugin(SyringeCore::CoreApi* api)
    {
        gfFileIOHandle handle;
        handle.read(this->path, Heaps::MenuInstance, 0);

        // Create the gfModule directly without intermediate storage
        this->module = gfModule::create(
            gfHeapManager::getHeap(Heaps::Syringe),
            handle.getBuffer(),
            handle.getSize());

        // Release the handle immediately after module creation
        handle.release();

        // Get plugin metadata from prolog
        this->metadata = reinterpret_cast<PluginPrologFN>(this->module->header->prologOffset)(api);

        char buff[10];
        this->metadata->VERSION.toString(this->metadata->VERSION, buff);
        OSReport("[Syringe] Loaded plugin (%s, v%s)\n", this->metadata->NAME, buff);

        this->enable = true;
        return module;
    }
    void Plugin::unloadPlugin()
    {
        // TECHNICALLY we should unlink first
        // but i don't see any issues?
        delete this->module;
        this->enable = false;
    }
}