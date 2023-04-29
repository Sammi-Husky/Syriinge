#pragma once

#include "ftp/ftp.h"

struct FAEntryInfo;
struct ftp_cmd;
int ftp_response(int client, int code, const char* fmt, ...);
ftp_cmd parse_cmd(char* buf, int len);
int get_args(char* dest, char* cmd);
int create_server(int port, int maxClients);
int send_file(int peer, char* file, int offset);
int recv_file(int client, char* filepath, int offset);
// recv wrapper that waits until socket is ready
int _recv(int socket, void* buffer, size_t len, int flags);
int build_stat(FAEntryInfo* entry, char* statbuf);
void buildPath(char* dest, char* file, char* cwd);