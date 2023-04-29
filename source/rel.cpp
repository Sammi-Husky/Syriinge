#include <OS/OSError.h>
#include <gf/gf_file_io.h>
#include <gf/gf_task.h>
#include <memory.h>
#include <net/net.h>

#include "css_hooks.h"
#include "ftp/ftp.h"
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

    void InitNetwork()
    {
        SOInitInfo info;
        info.allocator = SOAlloc;
        info.dealloc = SOFree;
        SOInit(&info);
        SOStartupEx(0x2bf20);
    }

    void _prolog()
    {
        // Run global constructors
        PFN_voidfunc* ctor;
        for (ctor = _ctors; *ctor; ctor++)
        {
            (*ctor)();
        }

        // initialize core systems
        SyringeCore::syInit();

        // Initialize the socket systems
        InitNetwork();

        FTP::start();             // FTP server
        NetLog::Init();           // Network logging interface
        CSSHooks::InstallHooks(); // Async RSP loading on the CSS
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