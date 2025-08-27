#pragma once

#include <gf/gf_scene.h>
#include <types.h>

// Forward Declarations
class gfModuleInfo;

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

    gfScene* getPrevScene() { return m_manager->m_prevScene; }
    gfScene* getCurrentScene() { return m_manager->m_currentScene; }
    gfScene* getNextScene() { return m_manager->m_nextScene; }
    gfSequence* getPrevSequence() { return m_manager->m_prevSequence; }
    gfSequence* getCurrentSequence() { return m_manager->m_currentSequence; }
    gfSequence* getNextSequence() { return m_manager->m_nextSequence; }
    s32 getMemoryLayout() const { return m_manager->m_memoryLayout; }

private:
    gfSceneManager* m_manager;
};
