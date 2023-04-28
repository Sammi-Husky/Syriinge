#include <fa/fa.h>
#include <net/net.h>
#include <string.h>

#include "VI/vi.h"
#include "debug.h"
#include "ftp/ftp.h"
#include "printf.h"

// recv wrapper that waits until socket is ready
int _recv(int socket, void* buffer, size_t len, int flags)
{
    while (!CanReceiveOnSocket(socket))
    {
        VIWaitForRetrace();
        continue;
    }

    return recv(socket, buffer, len, 0);
}

int ftp_response(int client, int code, const char* fmt, ...)
{
    char msgbuf[BUFF_SIZE];
    int len;
    if (code < 0)
    {
        len = sprintf(msgbuf, "%d- ", -code);
    }
    else
    {
        len = sprintf(msgbuf, "%d ", code);
    }
    va_list args;
    va_start(args, fmt);
    len += vsnprintf(msgbuf + len, sizeof(msgbuf) - len, fmt, args);
    va_end(args);

    return send(client, msgbuf, len, 0);
}
enum FTP_CMD parse_cmd(char* buf, int len)
{
    for (int i = 0; i < FTP_CMD_COUNT; i++)
    {
        ftp_cmd cmd = FTP_CMD_LIST[i];

        // iterate over characters in name (case insensitive)
        int j;
        for (j = 0; cmd.name[j] != '\0' && j < len; j++)
        {
            if (cmd.name[j] != buf[j] && cmd.name[j] != buf[j] - 32)
                break;
        }
        if (cmd.name[j] == '\0')
            return cmd.cmd;
    }
    return INVALID;
}
int get_args(char* dest, char* cmd)
{
    int i, j;

    // i == space character
    for (i = 0; cmd[i] != ' '; i++)
        ;

    // j == EOS or newline
    for (j = i; cmd[j] != '\r' && cmd[j] != '\n'; j++)
        ;

    // make sure we found a space and
    // move index to next char
    if (cmd[i++] != ' ')
        return -1;

    // copy all chars between space and newline
    strncpy(dest, &cmd[i], j - i);

    // make sure it's null terminated
    dest[j - i] = 0;

    return 0;
}
int create_server(int port, int maxClients)
{
    // create addr
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = SOHtoNl(INADDR_ANY);
    addr.sin_port = SOHtoNs(port);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        return -2;
    }
    if (listen(server, maxClients) < 0)
    {
        return -3;
    }
    return server;
}
int send_file(int client, char* file, int offset)
{
    FAHandle* handle = FAFopen(file, "r");
    if (!handle)
    {
        debug_log("Failed to open file: %s\n", file);
        return -1;
    }
    FAFseek(handle, offset, 0);

    int n;
    char filebuf[BUFF_SIZE + 1];
    while ((n = FAFread(filebuf, 1, BUFF_SIZE, handle)) > 0)
    {
        int sent = send(client, filebuf, n, 0);
        filebuf[n] = 0;
    }
    FAFclose(handle);

    return 0;
}
int recv_file(int client, char* filepath, int offset)
{
    FAHandle* handle = FAFopen(filepath, "w");
    if (!handle)
    {
        debug_log("Failed to create file: %s\n", filepath);
        return -1;
    }
    FAFseek(handle, offset, 0);

    int n;
    char filebuf[BUFF_SIZE + 1];
    while ((n = _recv(client, filebuf, BUFF_SIZE, 0)) > 0)
    {
        FAFwrite(filebuf, 1, n, handle);
    }

    FAFclose(handle);
    return 0;
}