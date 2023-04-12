#include "sy_core.h"
#include <OS/OSCache.h>
#include <OS/OSThread.h>
#include <VI/vi.h>
#include <gf/gf_archive.h>
#include <gf/gf_file_io.h>
#include <gf/gf_module.h>
#include <mu/mu_menu.h>
#include <mu/mu_sel_char_player_area.h>
#include <nw4r/g3d/g3d_resfile.h>
#include <printf.h>
#include <string.h>
#include <types.h>

using namespace nw4r::g3d;

namespace CSSHooks {

    struct loadParams {
        muSelCharPlayerArea* area;
        int charKind;
    };

    bool dataLoaded[4];
    loadParams* loadRequest;
    static void* loadThread(void* arg)
    {
        gfFileIOHandle handle;
        bool readInProgress = false;
        loadParams lp;

        while (true)
        {
            if (loadRequest != NULL)
            {
                // save a copy of the request
                lp.area = loadRequest->area;
                lp.charKind = loadRequest->charKind;

                void* dataAddr = (void*)*(u32*)(lp.area + 0x438);

                // reset texdata
                dataLoaded[0] = false;

                // if read is already in progress, cancel
                // it and start new read request
                if (readInProgress)
                {
                    handle.cancelRequest();
                    readInProgress = false;
                }

                // if handle is ready to accept commands
                if (handle.isReady())
                {
                    // handles conversions for poketrio and special slots
                    int id = muMenu::exchangeMuSelchkind2MuStockchkind(lp.charKind);
                    id = muMenu::getStockFrameID(id);

                    char filepath[0x80];
                    sprintf(filepath, "/menu/common/char_bust_tex/MenSelchrFaceB%02d0.brres", id);

                    // start the read process
                    handle.readRequest(filepath, dataAddr, 0, 0);

                    // clear read request for loadThread
                    delete loadRequest;
                    loadRequest = NULL;

                    readInProgress = true;
                }
            }

            // data is finished loading
            if (handle.isReady() && readInProgress)
            {
                dataLoaded[0] = true;
                readInProgress = false;

                // now that data is loaded, set pic appropriately
                int curCostume = *(int*)(lp.area + 0x1bc);

                // TODO: Need to set team info and player kind
                lp.area->setCharPic(lp.charKind, 1, curCostume, false, 0, 0);

                handle.release();
            }

            // sleep thread until next vsync
            VIWaitForRetrace();
        }

        return NULL;
    }

    ResFile* (*_getCharPicTexResFile)(muSelCharPlayerArea*, u32);
    ResFile* getCharPicTexResFile(muSelCharPlayerArea* playerArea, u32 param_2)
    {
        // if data hasn't been loaded, use transparent texture
        // and request data be loaded by load thread
        if (dataLoaded[0] == false)
        {
            loadRequest = new loadParams();
            loadRequest->area = playerArea;
            loadRequest->charKind = param_2;
            playerArea->setCharPic(0x28, 1, 0, false, 0, 0);
            return NULL;
        }
        else
        {
            void* dataAddr = (void*)*(u32*)(playerArea + 0x438);
            ResFile* resFile = (ResFile*)(playerArea + 0x43C);

            // flush cache
            DCFlushRange(dataAddr, 0x40000);

            // set data pointer in ResFile to point to filedata
            *(u32*)(playerArea + 0x43C) = *(u32*)(playerArea + 0x438);

            // init resFile and return
            nw4r::g3d::ResFile::Init(resFile);

            // to ensure we load more than just
            // the first hovered character
            dataLoaded[0] = false;

            return resFile;
        }
    }

    OSThread* threads;
    char** stacks;
    void createThreads()
    {
        // create threads and stacks on the heap
        // so we can destroy them later
        threads = new OSThread[4];
        stacks = new char*[4];
        for (int i = 0; i < 4; i++)
        {
            stacks[i] = new char[0x500];
        }

        // TODO: create one thread for each player
        // Currently, for testing it's only setup for player 1.
        // There is also no synchronization / locking happening.

        // for (int i = 0; i < 4; i++)
        // {
        int i = 0;
        OSCreateThread(&threads[i], loadThread, 0, stacks[i] + sizeof(stacks[i]), sizeof(stacks[i]), 31, 0);
        OSResumeThread(&threads[i]);
        // }
    }

    void InstallHooks()
    {
        // hook to load portraits from RSPs
        SyringeCore::syReplaceFunction(0x80693940,
                                       reinterpret_cast<void*>(getCharPicTexResFile),
                                       (void**)&_getCharPicTexResFile,
                                       10);

        SyringeCore::syHookFunction(0x8068363c, reinterpret_cast<void*>(createThreads), 10);
    }
} // namespace CSSHooks
