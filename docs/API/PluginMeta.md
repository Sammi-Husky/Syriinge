# PluginMeta
## Overview
Every plugin must define a `PluginMeta` structure. This structure is used to define the plugin version, the runtime version it was built for, the plugin author, etc.

The `_prolog` function should return a reference to this instance.

## Definition
``` cpp
struct PluginMeta {
    char NAME[20];
    char AUTHOR[20];
    Version VERSION;
    Version SY_VERSION;
};
```

## Example Usage
In following example, you can see two uses of the `Version` type that Syriinge provides. This type takes a string of the format `major.minor.rev`. 

The type has operator functions for comparing two instances against eachother.

We also use the `SYRINGE_VERSION` define for the core version / runtime. This define is present in every Syriinge SDK and should be used here to ensure your metadata always matches the SDK you are compiling with.
``` cpp
    const PluginMeta META = {
        "Sandbox",               // name
        "Sammi",                 // author
        Version("1.0.0"),        // version
        Version(SYRINGE_VERSION) // core version
    };

    const PluginMeta* __prolog(CoreApi* api)
    {
        // Run global constructors
        PFN_voidfunc* ctor;
        for (ctor = _ctors; *ctor; ctor++)
        {
            (*ctor)();
        }

        // Do hooking here with api
        // ...

        return &META;
    }
```
