# **API Overview**
## **CoreAPI**
In order for plugins to utilize Syriinge, it needs access to the Syriinge API. Syriinge provides API access to plugins by way of the [CoreAPI](CoreAPI.md) object. Plugins use this object to register hooks, subscribe to events, and more. 

The entrypoint to every plugin (by default called `_prolog`) takes a single parameter, which is a pointer to the [CoreAPI](CoreAPI.md) instance. 

To learn more about [CoreAPI](CoreAPI.md) and how to use it, take a look at the [CoreAPI](CoreAPI.md) page.

---

## **Plugin Metadata**
Every plugin contains a [PluginMeta](PluginMeta.md) definition. This structure is used by the developer to detail information such as the plugin version, the syriinge runtime it was built for, the plugin author, and other misc. information. 

You can learn more about the specific fields, what they do, and how to define the object in [PluginMeta](PluginMeta.md).

---

## **Hooks**
The API is the plugin developers main way of interacting with and registering hooks with Syriinge. Through this API, a developer can hook specific offsets within a function or even replace a function outright.

Syriinge provides two methods of hooking for each hook type. Static and Relative. The Relative hooking functions are used for hooking rels, as the name suggests, and hooks registered with it are applied when the target rel loads.

Static hooks are applied during the plugin load and are active immediately.

You can find a complete list of hooking functions in the [API Reference](CoreAPI.md/#static-hooking-methods). 

---

## **Events**
Syriinge offers developers the opportunity to register a callback function to be called when a number of events take place, such as when a rel is loaded. 

Using this system developers can detect the presence of other plugins, get access to rels as they are loaded, run routines on game events like match start, and more.

For more details on Events and Event Handling look at the [Event Handling Guide](../guides/event-handling.md)