# Quick Start Guide
This guide will get you familiar with the basic plumbing and steps required to get a bare-bones plugin written and loaded into the game as quickly as possible.

For the purposes of this guide, we are going to assume that you know the basics of managing a brawl build / modpack, and are familiar with how to add codes to your GCT.

---

## Installing the runtime
1.  Download a release build from the Project Repository, making sure to choose a "USER" zip. 
    - Alternatively, you can also [build from source](#)
2.  Mount your dolphin virtual SD, or insert your physical SD into your PC.
3.  Copy the contents of the previously downloaded zip file to the root of your build. (e.g `sd:/project+/`)
4.  Link the included `.asm` files into your RSBE01.txt file and then recompile the codeset. 
    - These codes MUST be in RSBE01 and **not** BOOST.
5.  Done! You should now have a functioning Syriinge install, ready to load plugins.

---

## Setting up the project
#### Required Components
 - DevKitPro
 - git

#### Syriinge Plugin Template
To streamline the creation of plugins, a [Template Repository](https://github.com/Sammi-Husky/SyriingePluginTemplate) has been created that scaffolds out a basic barebones "Hello World" example. For simplicity, we'll be using this repository to boostrap our development and explaining the individual parts as we go.

```
// Checkout the plugin repo
git checkout https://github.com/Sammi-Husky/SyriingePluginTemplate.git

// Move into the folder
cd SyriingePluginTemplate

// Build the plugin
make
```

If you followed the above steps and have the required components installed (and in your system path) you should end up with a built `.rel` plugin file to confirm that everything is working properly.

---

## Anatomy of a Plugin 
Now that we've got a solid boilerplate to base our plugin on, lets dive into each file and explain what they do.

### Makefile
To anyone versed in C/C++ development, Makefiles are an infamous if uniquitous part of your life. For the uninitiated, this file is responsible for holding the instructions for building your project. 

It compiles and links your code, and in our case uses `elf2rel` on it to generated a plugin file. There are also several configuration options contained in this file:

 - **TARGET**
    - This is the name of the plugin being built. The `.rel` file will be named whatever this is set to.
 - **RELID**
    - This denotes the ID of the `.rel` file.
 - **SOURCES** 
    - list of directories containing source files to be compiled.

### EXTRA.lst
Part of the build process for making `.rel` plugins is to partially link the compiled objects. This results in a great number of unresolved references. 

Thankfully, we can pass this partially linked binary into a tool called `elf2rel` along with a supplied mapping file (`.lst`) that maps symbols to addresses. 

`elf2rel` will use this map to resolve as many unresolved references as possible, and then convert the elf file into a `.rel` that is loadable by Syriinge.

### rel.lcf
This is the MWCC equivalent of the linkerscript. There is some metrowerks specific syntax going on here, but thankfully this doesn't need to be edited for 90% of usecases so we won't go into detail here.

### rel.cpp
This is the main entrypoint to the plugin. This file contains three functions:
 
 - _prolog
 - _epilog
 - _unresolved

#### _prolog
 The _prolog function is called automatically by the `sy_core` loader when loading plugins. 
 
 It recieves a single parameter, which is a pointer to the [CoreAPI](../API/CoreAPI.md) object. All hooking and API interactions are done via this object. `_prolog` is also responsible for iterating over and calling all the static initializer functions from the `.ctor` section. These are typically auto-generated.

#### _epilog
Similarly to the _prolog function, this function is called automatically when a `.rel` is unloaded by the game. This function is responsible for calling the destructors for all static objects in rel one by one. These destructors are in the `.dtor`, and are typically auto-generated.

#### _unresolved
This function is what is any remaining unresolved references that were not resolved in the `elf2rel` linking step will point to. Typically, this function is used to log the name of the module or file that the unresolved reference originates from.

---

## Further Reading
You should now have a good understanding of the boilerplate that is provided to you by the [Template Repository](https://github.com/Sammi-Husky/SyriingePluginTemplate). 

The next step is to build a simple "Hello World" application which you can follow our basic [Tutorial Guide](tutorial.md) to do.