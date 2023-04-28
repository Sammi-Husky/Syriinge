
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
#include "ftp/ftp_utils.h"

namespace FTP {

    bool running;
    int ctrl_socket = -1;
    int peer_socket = -1;
    int pasv_socket = -1;
    int data_socket = -1;
    int pasv_port;

    // addr for data connection
    sockaddr_in data_addr;
    u32 data_addr_len = sizeof(data_addr);

    // addr for pasv connection
    struct sockaddr_in pasv_addr;
    u32 pasv_addr_len = sizeof(pasv_addr);

    FTP_DATA_MODE dataMode = DATA_MODE_IMAGE; // default to binary
    FTP_XFER_MODE xferMode = XFER_MODE_PORT;  // default to port mode

    char buffer[BUFF_SIZE];
    char cmd_buffer[PATH_BUF_SIZE];
    char cwd[PATH_BUF_SIZE] = "/"; // initial CWD is root
    char tmp[0x20];
    int restoffset = 0;

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

    int build_stat(FAEntryInfo* entry, char* statbuf)
    {
        int end;
        if (entry->_flag & 0x10)
        {
            if (strcmp(entry->shortname, "..") == 0)
            {
                end = sprintf(statbuf, "type=pdir; %s\r\n", entry->shortname);
            }
            else if (strcmp(entry->shortname, ".") == 0)
            {
                end = sprintf(statbuf, "type=cdir; %s\r\n", entry->shortname);
            }
            else if (entry->name[0] == 0)
            {
                end = sprintf(statbuf, "type=dir; %s\r\n", entry->shortname);
            }
            else
            {
                end = sprintf(statbuf, "type=dir; %s\r\n", entry->name);
            }
        }
        else if (entry->_flag & 0x20)
        {
            if (entry->name[0] == 0)
            {
                end = sprintf(statbuf, "type=file;size=%d; %s\r\n", entry->size, entry->shortname);
            }
            else
            {
                end = sprintf(statbuf, "type=file;size=%d; %s\r\n", entry->size, entry->name);
            }
        }
        return end;
    }
    void handleLIST(int client)
    {
        data_socket = accept(pasv_socket, (struct sockaddr*)&data_addr, &data_addr_len);
        if (data_socket < 0)
        {
            debug_log("accept data client socket error\n");
            return;
        }
        ftp_response(client, 150, "Data Connection Accepted\r\n");

        FAEntryInfo info;
        if (strcmp(cwd, "/") == 0)
        {
            sprintf(cmd_buffer, "%s*", cwd);
        }
        else
        {
            sprintf(cmd_buffer, "%s/*", cwd);
        }
        int ret = FAFsfirst(cmd_buffer, 0x30, &info);
        if (ret != 0)
        {
            debug_log("Failed to intiate dir search (%s)\n", cmd_buffer);
            return;
        }

        char name[0x30];
        int len = 0;
        len = build_stat(&info, name);
        send(data_socket, name, len - 1, 0);

        while (FAFsnext(&info) == 0)
        {
            len = build_stat(&info, name);
            send(data_socket, name, len - 1, 0);
        }
        ftp_response(client, 226, "Transfer Complete\r\n");
        closesocket(data_socket);
        closesocket(pasv_socket);
    }
    void handleRETR(int client, char* buffer)
    {
        ftp_response(client, 150, "Binary Mode\r\n");
        data_socket = accept(pasv_socket, (struct sockaddr*)&data_addr, &data_addr_len);
        if (data_socket < 0)
        {
            debug_log("accept data client socket error\n");
            return;
        }
        get_args(tmp, buffer);
        sprintf(cmd_buffer, "%s/%s", cwd, tmp);
        if (send_file(data_socket, cmd_buffer, restoffset) >= 0)
        {
            ftp_response(client, 226, "Transfer complete\r\n");
            restoffset = 0;
        }
        closesocket(data_socket);
        closesocket(pasv_socket);
        return;
    }
    void handleSTOR(int client, char* buffer)
    {
        ftp_response(client, 150, "Binary Mode\r\n");
        data_socket = accept(pasv_socket, (struct sockaddr*)&data_addr, &data_addr_len);
        if (data_socket < 0)
        {
            debug_log("accept data client socket error\n");
            return;
        }
        get_args(tmp, buffer);
        sprintf(cmd_buffer, "%s/%s", cwd, tmp);
        if (recv_file(data_socket, cmd_buffer, restoffset) >= 0)
        {
            ftp_response(client, 226, "Transfer complete\r\n");
            restoffset = 0;
        }
        closesocket(data_socket);
        closesocket(pasv_socket);
        return;
    }
    void handleSession(int client)
    {
        // send ready response
        ftp_response(client, 220, "Welcome Aboard Captain\r\n");

        // copy client addr to pasv addr
        getsockname(client, (struct sockaddr*)&pasv_addr, &pasv_addr_len);

        // receive commands
        int n, i;
        while ((n = _recv(client, buffer, BUFF_SIZE, 0)) > 0)
        {
            if (!running)
                break;

            buffer[n] = '\0';

            // find newline or EOS
            for (i = 0; i < n; i++)
            {
                if (buffer[i] == '\n')
                    break;
            }

            // if EOS not newline
            if (buffer[i] != '\n')
            {
                debug_log("no line break found\n");
                break;
            }

            enum FTP_CMD cmd = parse_cmd(buffer, n);
            if (cmd < 0)
            {
                buffer[n - 2] = 0;
                debug_log("unknown cmd: %s\n", buffer);
                continue;
            }

            int retry;
            switch (cmd)
            {
            case USER:
                ftp_response(client, 230, "OK\r\n");
                break;
            case SYST:
                ftp_response(client, 215, "UNIX\r\n");
                break;
            case TYPE:
                if (buffer[5] == 'A')
                {
                    dataMode = DATA_MODE_ASCII;
                }
                else if (buffer[5] == 'I')
                {
                    dataMode = DATA_MODE_IMAGE;
                }
                else
                {
                    ftp_response(client, 500, "TYPE error, current type: %c\r\n", dataMode == 0 ? 'A' : 'I');
                    break;
                }
                ftp_response(client, 200, "OK\r\n");
                break;
            case PASV: {
                retry = 100;
                while (retry--)
                {
                    xferMode = XFER_MODE_PASV;
                    pasv_port = (rand() % 64512 + 1024);
                    pasv_socket = create_server(pasv_port, 1);
                    if (pasv_socket >= 0)
                        break;
                }
                if (pasv_socket < 0)
                {
                    debug_log("Could not create port for passive mode\n");
                    break;
                }
                int port = SONtoHs(pasv_port);
                ftp_response(client, 227, "Enter passive mode (%s,%u,%u)\r\n",
                             SOInetNtoA(pasv_addr.sin_addr),
                             port >> 8,
                             port & 0xff);

                debug_log("Entering PASV mode, port : %hu\n", pasv_port);
            }
            break;
            case PWD:
                ftp_response(client, 257, "\"%s\"\r\n", cwd);
                break;
            case CDUP:
                cdup(cwd);
                ftp_response(client, 200, "OK\r\n");
            case CWD:
                if (get_args(tmp, buffer) < 0)
                {
                    ftp_response(client, 500, "Error: Failed to set CWD\r\n");
                    break;
                }

                if (strcmp(tmp, "..") == 0)
                {
                    cdup(cwd);
                }
                else if (tmp[0] == '/')
                {
                    // if arg is absolute path
                    strncpy(cwd, tmp, 0x20);
                }
                else
                {
                    // if cwd is root
                    if (strcmp(cwd, "/") == 0)
                    {
                        sprintf(cmd_buffer, "/%s", tmp);
                    }
                    else
                    {
                        sprintf(cmd_buffer, "%s/%s", cwd, tmp);
                    }

                    // update current dir
                    strncpy(cwd, cmd_buffer, PATH_BUF_SIZE);
                }

                ftp_response(client, 200, "dir changed\r\n");
                break;
            case LIST:
                handleLIST(client);
                break;
            case RETR:
                handleRETR(client, buffer);
                break;
            case STOR:
                handleSTOR(client, buffer);
                break;
            case DELE:
                get_args(tmp, buffer);
                if (FARemove(tmp) != 0)
                {
                    ftp_response(client, 502, "Wrong Param: %s\r\n", buffer);
                    break;
                }
                ftp_response(client, 250, "OK\r\n");
                break;
            case REST:
                get_args(tmp, buffer);
                restoffset = atoi(tmp);
                ftp_response(client, 350, "REST, continue transfer at: 0x%x\r\n", restoffset);
                break;
            default:
                ftp_response(client, 502, "Not Implemented\r\n");
                debug_log("Unhandled request: %s\n", buffer);
                break;
            }
        }
    }

    static void* run(void*)
    {
        int server_socket = create_server(2121, 5);
        running = true;

        struct sockaddr_in server_addr;
        while (running)
        {
            while (!CanReceiveOnSocket(server_socket))
            {
                VIWaitForRetrace();
                continue;
            }

            u32 len = sizeof(server_addr);
            peer_socket = accept(server_socket, (struct sockaddr*)&server_addr, &len);

            if (peer_socket < 0)
            {
                debug_log("Error accepting connection\n");
                continue;
            }

            if (!running)
                break;

            debug_log("Connected: %s\n", SOInetNtoA(server_addr.sin_addr));
            handleSession(peer_socket);
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