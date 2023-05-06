
#include <OS/OSThread.h>
#include <VI/vi.h>
#include <extras.h>
#include <fa/fa.h>
#include <gf/gf_file_io.h>
#include <memory.h>
#include <pf/pf.h>
#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <strtoul.h>

#include "debug.h"
#include "ftp/ftp.h"
#include "ftp/ftp_session.h"
#include "ftp/ftp_utils.h"
#include "sy_core.h"

#define STACK_SIZE 0x4000

namespace FTP {

    bool running;

    // Function from sys-ftpd:
    // https://github.com/cathery/sys-ftpd/tree/master
    void cdup(char* cwd)
    {
        char *p, *slash = NULL;
        for (p = cwd; *p; ++p)
        {
            if (*p == '/')
                slash = p;
        }
        // truncate string at slash
        *slash = 0;

        // if slash was root, restore slash
        // but truncate afterwards
        if (strlen(cwd) == 0)
            strncat(cwd, "/", 2);
    }

    int handleCWD(FTPSession* session, char* args)
    {
        // remove trailing slash
        for (int i = ARG_BUF_SIZE; i > 0; i--)
        {
            if (args[i] == '/' && args[i + 1] == '\0')
            {
                args[i] = '\0';
                break;
            }
        }

        if (strcmp(args, "..") == 0)
        {
            strncpy(session->m_buffer, session->m_cwd, PATH_BUF_SIZE);
            cdup(session->m_buffer);
        }
        else if (args[0] == '/')
        {
            // if arg is absolute path
            strcpy(session->m_buffer, args);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        FAHandle* dir = FAOpendir(session->m_buffer);
        if (dir == 0)
        {
            ftp_response(session->m_ctrlSocket, 550, "Directory doesn't exist: %s\r\n", session->m_buffer);
            return 0;
        }
        FAClosedir(dir);

        // update current dir
        strncpy(session->m_cwd, session->m_buffer, PATH_BUF_SIZE);

        ftp_response(session->m_ctrlSocket, 250, "dir changed\r\n");
        return 0;
    }
    int handleMLSD(FTPSession* session, char* args)
    {
        // open connection
        session->OpenDataConnection();

        FAEntryInfo info;
        if (strcmp(session->m_cwd, "/") == 0)
        {
            // if args not empty copy to path, otherwise just use wildcard
            sprintf(session->m_buffer, "%s*", session->m_cwd);
        }
        else
        {
            sprintf(session->m_buffer, "%s/*", session->m_cwd);
        }
        int ret = FAFsfirst(session->m_buffer, 0x30, &info);
        if (ret != 0)
        {
            debug_log("Failed to intiate dir search (%s)\n", session->m_buffer);
            return -1;
        }

        int len = 0;
        len = build_stat_mlst(&info, session->m_buffer);
        send(session->m_dataSocket, session->m_buffer, len - 1, 0);

        while (FAFsnext(&info) == 0)
        {
            len = build_stat_mlst(&info, session->m_buffer);
            send(session->m_dataSocket, session->m_buffer, len - 1, 0);
        }

        // close connection
        ftp_response(session->m_ctrlSocket, 226, "Transfer Complete\r\n");
        session->CloseDataConnection();

        return 0;
    }
    int handleRETR(FTPSession* session, char* args)
    {
        session->OpenDataConnection();

        if (args[0] == '/')
        {
            // if arg is absolute path
            strcpy(session->m_buffer, args);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        if (send_file(session->m_dataSocket, session->m_buffer, session->m_restoffset) >= 0)
        {
            ftp_response(session->m_ctrlSocket, 226, "Transfer complete\r\n");
            session->m_restoffset = 0;
        }

        session->CloseDataConnection();
        return 0;
    }
    int handleSTOR(FTPSession* session, char* args)
    {
        session->OpenDataConnection();

        if (args[0] == '/')
        {
            // if arg is absolute path
            strcpy(session->m_buffer, args);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        if (recv_file(session->m_dataSocket, session->m_buffer, session->m_restoffset) >= 0)
        {
            ftp_response(session->m_ctrlSocket, 226, "Transfer complete\r\n");
            session->m_restoffset = 0;
        }

        session->CloseDataConnection();
        return 0;
    }
    int handleDELE(FTPSession* session, char* args)
    {
        if (args[0] == '/')
        {
            // if arg is absolute path
            strcpy(session->m_buffer, args);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        if (FARemove(session->m_buffer) != 0)
        {
            ftp_response(session->m_ctrlSocket, 502, "Wrong Param: DELE %s\r\n", session->m_buffer);
            return -1;
        }
        ftp_response(session->m_ctrlSocket, 250, "OK\r\n");

        return 0;
    }
    int handleREST(FTPSession* session, char* args)
    {
        session->m_restoffset = atoi(args);
        ftp_response(session->m_ctrlSocket, 350, "REST, continue transfer at: 0x%x\r\n", session->m_restoffset);
        return 0;
    }
    int handleCDUP(FTPSession* session, char* args)
    {
        cdup(session->m_cwd);
        ftp_response(session->m_ctrlSocket, 200, "OK\r\n");
        return 0;
    }
    int handlePWD(FTPSession* session, char* args)
    {
        ftp_response(session->m_ctrlSocket, 257, "\"%s\"\r\n", session->m_cwd);
        return 0;
    }
    int handleTYPE(FTPSession* session, char* args)
    {
        if (args[0] == 'A')
        {
            session->m_dataMode = DATA_MODE_ASCII;
        }
        else if (args[0] == 'I')
        {
            session->m_dataMode = DATA_MODE_IMAGE;
        }
        else
        {
            ftp_response(session->m_ctrlSocket, 500, "TYPE error, current type: %c\r\n", session->m_dataMode == 0 ? 'A' : 'I');
            return -1;
        }
        ftp_response(session->m_ctrlSocket, 200, "OK\r\n");
        return 0;
    }
    int handlePASV(FTPSession* session, char* args)
    {
        int retry = 100;
        int pasv_port;
        while (retry--)
        {
            session->m_xferMode = XFER_MODE_PASV;
            pasv_port = (rand() % 64512 + 1024);
            session->m_pasvSocket = create_server(pasv_port, 1);
            if (session->m_pasvSocket >= 0)
                break;
        }
        if (session->m_pasvSocket < 0)
        {
            debug_log("Could not create port for passive mode\n");
            return -1;
        }
        int port = SONtoHs(pasv_port);
        char _tmp[0x20];
        sprintf(_tmp, "%s", SOInetNtoA(session->m_pasvAddr.sin_addr));

        // replace period with commas
        // some clients require this
        for (char* p = _tmp; *p; ++p)
            if (*p == '.')
                *p = ',';

        ftp_response(session->m_ctrlSocket, 227, "Enter passive mode (%s,%u,%u)\r\n",
                     _tmp,
                     port >> 8,
                     port & 0xff);

        debug_log("Entering PASV mode, port : %hu\n", pasv_port);
        return 0;
    }
    int handleSYST(FTPSession* session, char* args)
    {
        ftp_response(session->m_ctrlSocket, 215, "UNIX\r\n");
        return 0;
    }
    int handleUSER(FTPSession* session, char* args)
    {
        ftp_response(session->m_ctrlSocket, 230, "OK\r\n");
        return 0;
    }
    int handleMKD(FTPSession* session, char* args)
    {
        if (args[0] == '/')
        {
            // if arg is absolute path
            strcpy(session->m_buffer, args);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        debug_log("New: %s\n", session->m_buffer);

        if (gfFileIO::gfFACreateDir(session->m_buffer) == 0)
        {
            ftp_response(session->m_ctrlSocket, 257, "Sucessfully created directory %s\r\n", args);
            return 0;
        }
        else
        {
            ftp_response(session->m_ctrlSocket, 550, "Failed to create directory\r\n");
            return -1;
        }
    }
    int handleRMD(FTPSession* session, char* args)
    {
        if (args[0] == '/')
        {
            // if arg is absolute path
            strcpy(session->m_buffer, args);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }
        int err = FARemove(session->m_buffer);
        ftp_response(session->m_ctrlSocket, 250, "OK\r\n");
        return err;
    }
    int handleRNFR(FTPSession* session, char* args)
    {
        if (args[0] == '/')
        {
            // if arg is absolute path
            strncpy(session->m_buffer, args, ARG_BUF_SIZE);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        ftp_response(session->m_ctrlSocket, 350, "RNFR\r\n");
        return 0;
    }
    int handleRNTO(FTPSession* session, char* args)
    {
        char tmp[PATH_BUF_SIZE];
        if (args[0] == '/')
        {
            // if arg is absolute path
            strncpy(tmp, args, ARG_BUF_SIZE);
        }
        else
        {
            buildPath(tmp, args, session->m_cwd);
        }

        pfstat st;
        FAFstat(session->m_buffer, &st);
        if (st.flags & 0x10)
        {
            renameFolder(tmp, session->m_buffer);
        }
        else if (st.flags & 0x20)
        {
            renameFile(tmp, session->m_buffer);
        }
        ftp_response(session->m_ctrlSocket, 250, "RNTO success\r\n");
        return 0;
    }
    int handleFEAT(FTPSession* session, char* args)
    {
        ftp_response(session->m_ctrlSocket, -211,
                     "\r\n"
                     " PASV\r\n"
                     " MLST\r\n"
                     " MLSD\r\n"
                     "\r\n"
                     "211 End\r\n");
        return 0;
    }
    int handleOPTS(FTPSession* session, char* args)
    {
        if (stricmp(args, "UTF8") == 0 || stricmp(args, "UTF8 ON") == 0)
        {
            ftp_response(session->m_ctrlSocket, 200, "OK\r\n");
            return 0;
        }

        ftp_response(session->m_ctrlSocket, 504, "Invalid Argument\r\n");
        return -1;
    }
    int handleLIST(FTPSession* session, char* args)
    {
        // open connection
        session->OpenDataConnection();

        FAEntryInfo info;
        if (strcmp(session->m_cwd, "/") == 0)
        {
            // if args not empty copy to path, otherwise just use wildcard
            sprintf(session->m_buffer, "%s*", session->m_cwd);
        }
        else
        {
            sprintf(session->m_buffer, "%s/*", session->m_cwd);
        }
        int ret = FAFsfirst(session->m_buffer, 0x30, &info);
        if (ret != 0)
        {
            debug_log("Failed to intiate dir search (%s)\n", session->m_buffer);
            return -1;
        }

        int len = 0;
        len = build_stat_list(&info, session->m_buffer);
        send(session->m_dataSocket, session->m_buffer, len - 1, 0);

        while (FAFsnext(&info) == 0)
        {
            len = build_stat_list(&info, session->m_buffer);
            send(session->m_dataSocket, session->m_buffer, len - 1, 0);
        }

        // close connection
        ftp_response(session->m_ctrlSocket, 226, "Transfer Complete\r\n");
        session->CloseDataConnection();

        return 0;
    }
    int handleNOOP(FTPSession* session, char* args)
    {
        ftp_response(session->m_ctrlSocket, 200, "OK\r\n");
        return 0;
    }
    int handleSITE(FTPSession* session, char* args)
    {
        ftp_response(session->m_ctrlSocket, 202, "Not Supported\r\n");
        return 0;
    }
    int handleSIZE(FTPSession* session, char* args)
    {
        pfstat st;

        if (args[0] == '/')
        {
            // if arg is absolute path
            strncpy(session->m_buffer, args, ARG_BUF_SIZE);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        if (FAFstat(session->m_buffer, &st) == 0)
        {
            ftp_response(session->m_ctrlSocket, 213, "%d\r\n", st.filesize);
            return 0;
        }

        ftp_response(session->m_ctrlSocket, 550, "Failed to get filesize\r\n");
        return -1;
    }
    int handleMDTM(FTPSession* session, char* args)
    {
        pfstat st;
        if (args[0] == '/')
        {
            // if arg is absolute path
            strncpy(session->m_buffer, args, ARG_BUF_SIZE);
        }
        else
        {
            buildPath(session->m_buffer, args, session->m_cwd);
        }

        if (FAFstat(session->m_buffer, &st) == 0)
        {
            u16 date = st.modifiedDate;
            u16 time = st.modifiedTime;

            int year, month, day;
            year = 1980 + (date >> 9);
            month = (date & 0x1E0) >> 5;
            day = date & 0x1F;

            int hour, min, sec;
            hour = time >> 11;
            min = (time & 0x7E0) >> 5;
            sec = (time & 0x1F) * 2;

            ftp_response(session->m_ctrlSocket, 213, "%4d%02d%02d%02d%02d%02d\r\n", year, month, day, hour, min, sec);
            return 0;
        }

        ftp_response(session->m_ctrlSocket, 550, "Error getting datetime\r\n");
        return -1;
    }

    FTPCommand FTP_CMD_LIST[] = {
        { "USER", handleUSER },
        { "RETR", handleRETR },
        { "STOR", handleSTOR },
        { "REST", handleREST },
        { "DELE", handleDELE },
        { "PWD", handlePWD },
        { "CWD", handleCWD },
        { "CDUP", handleCDUP },
        { "LIST", handleLIST },
        { "MLSD", handleMLSD },
        { "MLST", handleMLSD },
        { "TYPE", handleTYPE },
        { "PASV", handlePASV },
        { "FEAT", handleFEAT },
        { "SYST", handleSYST },
        { "MKD", handleMKD },
        { "RMD", handleRMD },
        { "RNFR", handleRNFR },
        { "RNTO", handleRNTO },
        { "OPTS", handleOPTS },
        { "NOOP", handleNOOP },
        { "SITE", handleSITE },
        { "SIZE", handleSIZE },
        { "MDTM", handleMDTM },
        { "", NULL }
    };

    FTPCommand* parse_cmd(const char* buf, int len)
    {
        for (int i = 0; FTP_CMD_LIST[i].handler != NULL; i++)
        {
            FTPCommand cmd = FTP_CMD_LIST[i];

            // iterate over characters in name (case insensitive)
            int j;
            for (j = 0; cmd.name[j] != '\0' && j < len; j++)
            {
                if (cmd.name[j] != buf[j] && cmd.name[j] != buf[j] - 32)
                    break;
            }
            if (cmd.name[j] == '\0')
                return &FTP_CMD_LIST[i];
        }

        return NULL;
    }

    int curSession = -1;
    FTPSession* sessions[5] = { 0 };
    void handleSession(FTPSession* session)
    {
        // receive commands
        int buffernext = 0;
        char cmd_buf[DATA_BUF_SIZE];
        int pasv_port;
        int n = _recv(session->m_ctrlSocket, cmd_buf, DATA_BUF_SIZE, 0);
        cmd_buf[n] = 0;

        if (n > 0)
        {
            // there may be more than one command to process
            // since we last handled this session
            char* buffer = cmd_buf + buffernext;
            while (strlen(buffer + buffernext) > 0)
            {
                // find end of command and set buffernext
                // to point to the next cmd if it exists
                int i;
                for (i = 0; i < n; i++)
                {
                    if (buffer[i] == '\r')
                    {
                        buffernext = i + 2;
                        break;
                    }
                    else if (buffer[i] == '\n')
                    {
                        buffernext = i + 1;
                        break;
                    }
                }

                FTPCommand* cmd = parse_cmd(buffer, i);
                if (cmd == NULL)
                {
                    debug_log("unknown cmd: %s\n", buffer);
                    ftp_response(session->m_ctrlSocket, 502, "Not implemented: %s\r\n", buffer);
                    return;
                }

                char args[ARG_BUF_SIZE] = { 0 };
                get_args(args, buffer);
                cmd->handler(session, args);
            }
            buffernext = 0;
        }
        else if (n == 0)
        {
            delete session;
            sessions[curSession--] = 0;
            debug_log("Connection was closed. Next session: %d\n", curSession);
            return;
        }
    }

    static void* run(void*)
    {
        int server_socket = create_server(2121, 5);
        running = true;

        // wait for incoming connection
        while (!CanReceiveOnSocket(server_socket))
            VIWaitForRetrace();

        while (running)
        {
            if (CanReceiveOnSocket(server_socket))
            {
                FTPSession* session = new (Heaps::Network) FTPSession(server_socket);
                if (session->m_ctrlSocket < 0)
                {
                    delete session;
                }
                else
                {
                    sessions[++curSession] = session;
                }
            }

            if (!running)
                break;

            // This is not how we should be doing this
            for (int i = 0; i < 5; i++)
            {
                if (sessions[i] != NULL)
                    handleSession(sessions[i]);
            }
        }

        return NULL;
    }

    // clang-format off
    asm void PFFILE_p_remove_hook(){
        nofralloc
        lwz r4, 0(r5) // original instruction
        andi. r0, r0, 0x10
        beq _branchback
        lwz  r0, 0xA0(r1)
        cmpwi r0, 0
        beq _branchback
        _branchend:
            li r3, 0xb // error
            lis r12, 0x803e
            ori r12, r12, 0x4dfc
            mtctr r12
            bctr

        _branchback:
            lbz r0, 0x2c4(r1)
            lis r12, 0x803e
            ori r12, r12, 0x4d2c
            mtctr r12
            bctr
    }
    // clang-format on

    OSThread thread;
    char* stack;
    void start()
    {
        // allows FARemove to delete directories
        *(u32*)0x803e4d30 = 0x70000009; // ignore directory attribute
        *(u32*)0x803e1904 = 0x60000000; // free FAT entry even if directory

        // Hook to make FARemove return an error if attempting
        // to delete a non-empty directory
        SyringeCore::sySimpleHook(0x803e4d28, reinterpret_cast<void*>(PFFILE_p_remove_hook));

        // create stack on Network
        // heap to save space in ours
        stack = new (Heaps::Network) char[STACK_SIZE];

        OSCreateThread(&thread, run, NULL, stack + STACK_SIZE, STACK_SIZE, 31, 0);
        OSResumeThread(&thread);
    }
}