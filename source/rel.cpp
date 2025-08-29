#include <OS/OSError.h>
#include <SO/SOBasic.h>

#include "sy_core.hpp"

extern "C" {
typedef void (*PFN_voidfunc)();
__attribute__((section(".ctors"))) extern PFN_voidfunc _ctors[];
__attribute__((section(".ctors"))) extern PFN_voidfunc _dtors[];

void _prolog();
void _epilog();
void _unresolved();
}

// void InitNetwork()
// {
//     SOInitInfo info = {
//         SOAlloc,
//         SOFree
//     };
//     SOInit(&info);
//     SOStartupEx(0x2bf20);
// }

void _prolog()
{
    OSReport("[Syringe] Initializing. (ver. %s)\n", SYRINGE_VERSION);

    // Run global constructors
    PFN_voidfunc* ctor;
    for (ctor = _ctors; *ctor; ctor++)
    {
        (*ctor)();
    }

    // initialize core systems
    SyringeCore::syInit();

    // Initialize the socket systems
    // THIS SLOWS DOWN CONSOLE BOOT BY 40 SECONDS
    // InitNetwork();

    int num = SyringeCore::syLoadPlugins("plugins");

    // try and apply rel hooks to modules which
    // were already loaded before loading plugins
    // SyringeCore::hookLoadedModules();
}

void _epilog()
{
    // run the global destructors
    PFN_voidfunc* dtor;
    for (dtor = _dtors; *dtor; dtor++)
    {
        (*dtor)();
    }
}

void _unresolved(void)
{
}