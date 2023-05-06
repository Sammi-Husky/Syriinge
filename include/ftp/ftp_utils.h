#pragma once

#include "ftp/ftp.h"

struct FAEntryInfo;
struct ftp_cmd;
int ftp_response(int client, int code, const char* fmt, ...);
int get_args(char* dest, const char* cmd);
int create_server(int port, int maxClients);
int send_file(int peer, const char* file, int offset);
int recv_file(int client, const char* filepath, int offset);
// recv wrapper that waits until socket is ready
int _recv(int socket, void* buffer, size_t len, int flags);
int build_stat_mlst(FAEntryInfo* entry, char* statbuf);
int build_stat_list(FAEntryInfo* entry, char* statbuf);
void buildPath(char* dest, const char* file, const char* cwd);
int renameFile(const char* newName, const char* oldName);
int renameFolder(const char* newName, const char* oldName);