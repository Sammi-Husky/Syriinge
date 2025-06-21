# CoreAPI
## Overview
The CoreAPI instance is the primary contact point between the Syriinge core and plugins. Plugins use this instance to register hooks and subscribe to game or syriinge events.

Every plugin receives a pointer to this API object as a parameter to the main plugin entrypint (`_prolog`). 

---

## Static Hooking vs Rel Hooking
There are two ways to inject hooks with Syriinge. Static hooking and Rel Hooking.

**Static Hooks** are hooks which are applied immediately upon plugin load and are never re-applied. This is primarily used for hooking `main.dol` as it never changes.

**Rel Hooks** are hooks that get applied any time the target rel is loaded into memory by the game.

---

## Static Hooking Methods
 - [syInlineHook](hooks/inline-hook.md)
 - [sySimpleHook](hooks/simple-hook.md)
 - [syReplaceFunc](hooks/replace-func.md)

## Relative Hooking Methods
 - [syInlineHookRel](hooks/inline-hook-rel.md)
 - [sySimpleHookRel](hooks/simple-hook-rel.md)
 - [syReplaceFuncRel](hooks/replace-func-rel.md)

## Event Handling Methods
 - [moduleLoadEventSubscribe](events/ModuleLoadEvent.md)