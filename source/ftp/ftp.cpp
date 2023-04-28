
#include <OS/OSThread.h>
#include <VI/vi.h>
#include <fa/fa.h>
#include <memory.h>
#include <net/net.h>
#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <strtoul.h>

#include "debug.h"
#include "ftp/ftp.h"
#include "ftp/ftp_session.h"
#include "ftp/ftp_utils.h"

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
        if (strcmp(args, "..") == 0)
        {
            cdup(session->m_cwd);
        }
        else if (args[0] == '/')
        {
            // if arg is absolute path
            strncpy(session->m_cwd, args, 0x20);
        }
        else
        {
            // if cwd is root
            if (strcmp(session->m_cwd, "/") == 0)
            {
                sprintf(session->m_buffer, "/%s", args);
            }
            else
            {
                sprintf(session->m_buffer, "%s/%s", session->m_cwd, args);
            }

            // update current dir
            strncpy(session->m_cwd, session->m_buffer, PATH_BUF_SIZE);
        }

        ftp_response(session->m_ctrlSocket, 200, "dir changed\r\n");
        return 0;
    }
    int handleLIST(FTPSession* session, char* args)
    {
        // open connection
        session->OpenDataConnection();

        FAEntryInfo info;
        if (strcmp(session->m_cwd, "/") == 0)
        {
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
        len = build_stat(&info, session->m_buffer);
        send(session->m_dataSocket, session->m_buffer, len - 1, 0);

        while (FAFsnext(&info) == 0)
        {
            len = build_stat(&info, session->m_buffer);
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

        sprintf(session->m_buffer, "%s/%s", session->m_cwd, args);
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

        sprintf(session->m_buffer, "%s/%s", session->m_cwd, args);
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
        if (FARemove(args) != 0)
        {
            ftp_response(session->m_ctrlSocket, 502, "Wrong Param: DELE %s\r\n", args);
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
        ftp_response(session->m_ctrlSocket, 227, "Enter passive mode (%s,%u,%u)\r\n",
                     SOInetNtoA(session->m_pasvAddr.sin_addr),
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
        { "TYPE", handleTYPE },
        { "PASV", handlePASV },
        { "SYST", handleSYST },
        { "", NULL }
    };

    FTPCommand* parse_cmd(char* buf, int len)
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
        char cmd_buf[BUFF_SIZE];
        int pasv_port;
        int n = _recv(session->m_ctrlSocket, cmd_buf, BUFF_SIZE, 0);
        cmd_buf[n] = 0;

        if (n > 0)
        {
            // there may be more than one command to process
            // since we last handled this session
            char* buffer = cmd_buf + buffernext;
            while (strlen(buffer + buffernext) > 0)
            {
                // find CRLF and remove it
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

                char args[0x80];
                get_args(args, buffer);
                int err = cmd->handler(session, args);

                if (err)
                {
                    ftp_response(session->m_ctrlSocket, 500, "Error handling cmd: %s\r\n", cmd->name);
                }
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

    OSThread thread;
    char stack[0x2000];
    void main()
    {
        // initialize socket system
        SOInitInfo info;
        info.allocator = SOAlloc;
        info.dealloc = SOFree;
        SOInit(&info);
        SOStartupEx(0x2bf20);

        OSCreateThread(&thread, run, NULL, stack + sizeof(stack), sizeof(stack), 31, 0);
        OSResumeThread(&thread);
    }
}