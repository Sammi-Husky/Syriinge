#include <OS/OSError.h>
#include <gf/gf_file_io.h>
#include <gf/gf_task.h>
#include <memory.h>

#include "css_hooks.h"
#include "net_log.h"
#include "sy_core.h"

namespace Syringe {

    extern "C" {
    typedef void (*PFN_voidfunc)();
    __attribute__((section(".ctors"))) extern PFN_voidfunc _ctors[];
    __attribute__((section(".ctors"))) extern PFN_voidfunc _dtors[];

    void _prolog();
    void _epilog();
    void _unresolved();
    }

    void _prolog()
    {
        // Run global constructors
        PFN_voidfunc* ctor;
        for (ctor = _ctors; *ctor; ctor++)
        {
            (*ctor)();
        }
        SyringeCore::syInit();

        NetLog::Init();

        CSSHooks::InstallHooks();
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

}