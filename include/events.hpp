#pragma once

#include <gf/gf_scene.h>
#include <sr/sr_common.h>
#include <types.h>

// Forward Declarations
class gfModuleInfo;
class gfScene;
class gfSceneManager;

struct Event {
public:
    enum EventType {
        ModuleLoad,
        ModuleUnload,
        SceneChange,
        INVALID
    };
    EventType type;
    union {
        gfModuleInfo* moduleInfo;
        gfSceneManager* sceneManager;
    } payload;

    EventType getType() const { return type; }
    gfModuleInfo* getModuleInfo() { return payload.moduleInfo; }

    /**
     * @brief Filename of the module being loaded (e.g. "ft_mario.rel").
     */
    const char* getModuleName() const;
    /**
     * @brief Heap pointer the game loaded this module into
     */
    void* getHeap() const;

    gfScene* getPrevScene() const;
    gfScene* getCurrentScene() const;
    gfScene* getNextScene() const;
    gfSequence* getPrevSequence() const;
    gfSequence* getCurrentSequence() const;
    gfSequence* getNextSequence() const;
    s32 getMemoryLayout() const;
};
