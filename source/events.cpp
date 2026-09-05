#include "events.hpp"
#include <gf/gf_module.h>

gfScene* Event::getPrevScene() const { return payload.sceneManager->m_prevScene; }
gfScene* Event::getCurrentScene() const { return payload.sceneManager->m_currentScene; }
gfScene* Event::getNextScene() const { return payload.sceneManager->m_nextScene; }
gfSequence* Event::getPrevSequence() const { return payload.sceneManager->m_prevSequence; }
gfSequence* Event::getCurrentSequence() const { return payload.sceneManager->m_currentSequence; }
gfSequence* Event::getNextSequence() const { return payload.sceneManager->m_nextSequence; }
s32 Event::getMemoryLayout() const { return payload.sceneManager->m_memoryLayout; }

// m_moduleName is the first char of the module's inline name buffer.
const char* Event::getModuleName() const { return &payload.moduleInfo->m_moduleName; }
void* Event::getHeap() const { return payload.moduleInfo->m_heap; }
