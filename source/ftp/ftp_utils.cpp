#include <fa/fa.h>
#include <net/net.h>
#include <pf/pf.h>
#include <string.h>

#include "VI/vi.h"
#include "datetime.h"
#include "debug.h"
#include "ftp/ftp.h"
#include "printf.h"

// recv wrapper that waits until socket is ready
int _recv(int socket, void* buffer, size_t len, int flags)
{
    if (CanReceiveOnSocket(socket))
        return recv(socket, buffer, len, 0);
    else
        return -1;
}

int ftp_response(int client, int code, const char* fmt, ...)
{
    char msgbuf[DATA_BUF_SIZE];
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
int get_args(char* dest, const char* cmd)
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
int send_file(int client, const char* file, int offset)
{
    FAHandle* handle = FAFopen(file, "r");
    if (!handle)
    {
        debug_log("Failed to open file: %s\n", file);
        return -1;
    }
    FAFseek(handle, offset, 0);

    int n;
    char filebuf[DATA_BUF_SIZE + 1];
    while ((n = FAFread(filebuf, 1, DATA_BUF_SIZE, handle)) > 0)
    {
        int sent = send(client, filebuf, n, 0);
        filebuf[n] = 0;
    }
    FAFclose(handle);

    return 0;
}
int recv_file(int client, const char* filepath, int offset)
{
    FAHandle* handle = FAFopen(filepath, "w");
    if (!handle)
    {
        debug_log("Failed to create file: %s\n", filepath);
        return -1;
    }
    FAFseek(handle, offset, 0);

    int n;
    char filebuf[DATA_BUF_SIZE + 1];
    while ((n = _recv(client, filebuf, DATA_BUF_SIZE, 0)) > 0)
    {
        FAFwrite(filebuf, 1, n, handle);
    }

    FAFclose(handle);
    return 0;
}
void build_stat_datetime(char* dst, u16 date, u16 time)
{
    u16 tdDate, tdTime;

    char tmp[0x10]; // for month str
    dosDateToS(tmp, date);

    PFENT_getcurrentDateTimeForEnt(&tdDate, &tdTime);

    // date
    int year = 1980 + (date >> 9);
    int month = (date & 0x1E0) >> 5;
    int day = date & 0x1F;

    // date
    int tdYear = 1980 + (tdDate >> 9);
    int tdMonth = (tdDate & 0x1E0) >> 5;
    int tdDay = tdDate & 0x1F;

    int difference = ((tdYear - year) * 12) + (tdMonth - month);
    if (difference > 6)
    {
        sprintf(dst, "%s %4d", tmp, year);
    }
    else
    {
        // time
        int hour = time >> 11;
        int min = (time & 0x7E0) >> 5;
        int sec = (time & 0x1F) * 2;
        sprintf(dst, "%s %02d:%02d", tmp, hour, min);
    }
}
int build_stat_mlst(FAEntryInfo* entry, char* statbuf)
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
int build_stat_list(FAEntryInfo* entry, char* statbuf)
{
    char fmt[] = "%crw-rw-rw- 1 root root %d %s %s\r\n";
    char date[0x10];
    char time[0x8];

    char datetime[0x20];
    build_stat_datetime(datetime, entry->modifiedDate, entry->modifiedTime);

    int end;
    if (entry->_flag & 0x10)
    {
        if (strcmp(entry->shortname, ".") == 0 || strcmp(entry->shortname, "..") == 0 || entry->name[0] == 0)
        {
            end = sprintf(statbuf, fmt, 'd', 0, datetime, entry->shortname);
        }
        else
        {
            end = sprintf(statbuf, fmt, 'd', 0, datetime, entry->name);
        }
    }
    else if (entry->_flag & 0x20)
    {
        if (entry->name[0] == 0)
        {
            end = sprintf(statbuf, fmt, '-', entry->size, datetime, entry->shortname);
        }
        else
        {
            end = sprintf(statbuf, fmt, '-', entry->size, datetime, entry->name);
        }
    }
    return end;
}
void buildPath(char* dest, const char* file, const char* cwd)
{
    if (strcmp(cwd, "/") == 0)
    {
        sprintf(dest, "/%s", file);
    }
    else
    {
        sprintf(dest, "%s/%s", cwd, file);
    }
}
int renameFile(const char* newName, const char* oldName)
{
    char filebuf[DATA_BUF_SIZE];
    pfstat st;
    FAFstat(oldName, &st);

    FAHandle* src = FAFopen(oldName, "r");
    FAHandle* dst = FAFopen(newName, "w");
    int i = st.filesize;
    while (i > 0)
    {
        if (i > DATA_BUF_SIZE)
        {
            FAFread(filebuf, 1, DATA_BUF_SIZE, src);
            FAFwrite(filebuf, 1, DATA_BUF_SIZE, dst);
            i -= DATA_BUF_SIZE;
        }
        else
        {
            FAFread(filebuf, 1, i, src);
            FAFwrite(filebuf, 1, i, dst);
            break;
        }
    }
    FAFclose(src);
    FAFclose(dst);
    FARemove(oldName);

    return 0;
}
int renameFolder(const char* newName, const char* oldName)
{
    int err;
    PFSTR str;
    PFENT_ITER iter;
    PFENT entry;
    char dirent[0x20];
    err = PFSTR_InitStr(&str, oldName, 0x1);
    if (err == 0)
    {
        PFVOL* vol = PFPATH_GetVolumeFromPath(&str);
        PF_enterCritical(&vol->_0x1f90);
        vol->_0x1644 = 0x0;

        // copy old dirent and delete it
        PFENT_ITER_GetEntryOfPath(&iter, &entry, vol, &str, 0x0);
        PFSEC_ReadData(entry.cache, &dirent, entry.sector, entry.offset, 0x20, &err, 0x0);
        PFENT_RemoveEntry(&entry, &iter);

        // create new directory
        err = PFSTR_InitStr(&str, newName, 0x1);
        PFDIR_p_mkdir(vol, &str, 0x0, NULL);

        // write sector and filesize from old
        // directory entry to our new one
        PFENT_ITER_GetEntryOfPath(&iter, &entry, vol, &str, 0x0);
        PFFAT_FreeChain(&iter._0x8, entry._0x234, 0xffffffff, entry._0x228);
        PFSEC_WriteData(entry.cache, &dirent[0x14], entry.sector, entry.offset + 0x14, 0x2, &err, 0x0);
        PFSEC_WriteData(entry.cache, &dirent[0x1A], entry.sector, entry.offset + 0x1A, 0x2, &err, 0x0);
        PFSEC_WriteData(entry.cache, &dirent[0x1C], entry.sector, entry.offset + 0x1c, 0x4, &err, 0x0);

        err = PFCACHE_FlushFATCache(vol);
        err = PFCACHE_FlushDataCacheSpecific(vol, 0x0);
        PF_exitCritical(&vol->_0x1f90);
    }

    return PFAPI_convertReturnValue(err);
}
