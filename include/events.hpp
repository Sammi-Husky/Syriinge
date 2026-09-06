#pragma once

#include <types.h>

class gfModuleInfo;
class gfScene;
class gfSceneManager;
class gfSequence;

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
    const char* getModuleName() const;
    void* getHeap() const;
    gfScene* getPrevScene() const;
    gfScene* getCurrentScene() const;
    gfScene* getNextScene() const;
    gfSequence* getPrevSequence() const;
    gfSequence* getCurrentSequence() const;
    gfSequence* getNextSequence() const;
    s32 getMemoryLayout() const;
};

namespace SyringeCore {
    typedef void (*EventHandlerFN)(Event& event);
}
