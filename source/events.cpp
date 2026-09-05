#include "events.hpp"
#include <gf/gf_module.h>

gfScene* SceneChangeEvent::getPrevScene() { return m_manager->m_prevScene; }
gfScene* SceneChangeEvent::getCurrentScene() { return m_manager->m_currentScene; }
gfScene* SceneChangeEvent::getNextScene() { return m_manager->m_nextScene; }
gfSequence* SceneChangeEvent::getPrevSequence() { return m_manager->m_prevSequence; }
gfSequence* SceneChangeEvent::getCurrentSequence() { return m_manager->m_currentSequence; }
gfSequence* SceneChangeEvent::getNextSequence() { return m_manager->m_nextSequence; }
s32 SceneChangeEvent::getMemoryLayout() const { return m_manager->m_memoryLayout; }

// m_moduleName is the first char of the module's inline name buffer.
const char* ModuleLoadEvent::getModuleName() { return &m_moduleInfo->m_moduleName; }
void* ModuleLoadEvent::getHeap() { return m_moduleInfo->m_heap; }

const char* ModuleUnloadEvent::getModuleName() { return &m_moduleInfo->m_moduleName; }