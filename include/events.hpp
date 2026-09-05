#pragma once

#include <gf/gf_scene.h>
#include <sr/sr_common.h>
#include <types.h>

// Forward Declarations
class gfModuleInfo;
class gfScene;
class gfSceneManager;

class Event {
public:
    enum EventType {
        ModuleLoad,
        ModuleUnload,
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

    /**
     * @brief Filename of the module being loaded (e.g. "ft_mario.rel").
     */
    const char* getModuleName();
    /**
     * @brief Heap pointer the game loaded this module into
     */
    void* getHeap();

private:
    gfModuleInfo* m_moduleInfo;
};

/**
 * @brief Fired when a game module is unloaded.
 */
class ModuleUnloadEvent : public Event {
public:
    ModuleUnloadEvent(gfModuleInfo* info) : m_moduleInfo(info) {}
    virtual EventType getType() const { return Event::ModuleUnload; }
    gfModuleInfo* getModuleInfo() { return m_moduleInfo; }

    /**
     * @brief Filename of the module being unloaded (e.g. "ft_mario.rel").
     */
    const char* getModuleName();

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
