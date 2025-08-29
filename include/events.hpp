#pragma once

#include <gf/gf_scene.h>
#include <types.h>

// Forward Declarations
class gfModuleInfo;
class gfScene;
class gfSceneManager;

class Event {
public:
    enum EventType {
        ModuleLoad,
        SceneChange,
        INVALID
    };
    virtual ~Event() {};
    virtual EventType getType() const = 0;
};

class ModuleLoadEvent : public Event {
public:
    ModuleLoadEvent(gfModuleInfo* info) : m_moduleInfo(info) {}
    virtual EventType getType() const { return Event::ModuleLoad; }
    gfModuleInfo* getModuleInfo() { return m_moduleInfo; }

private:
    gfModuleInfo* m_moduleInfo;
};

class SceneChangeEvent : public Event {
public:
    SceneChangeEvent(gfSceneManager* manager) : m_manager(manager) {}
    virtual EventType getType() const { return Event::SceneChange; }

    gfScene* getPrevScene();
    gfScene* getCurrentScene();
    gfScene* getNextScene();
    gfSequence* getPrevSequence();
    gfSequence* getCurrentSequence();
    gfSequence* getNextSequence();
    s32 getMemoryLayout() const;

private:
    gfSceneManager* m_manager;
};
