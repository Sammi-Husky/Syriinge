#pragma once

#include "ftp/vars.h"

int ftp_response(int client, int code, const char* fmt, ...);
FTP_CMD parse_cmd(char* buf, int len);
int get_args(char* dest, char* cmd);
int create_server(int port, int maxClients);
int send_file(int peer, char* file, int offset);
int recv_file(int client, char* filepath, int offset);
// recv wrapper that waits until socket is ready
int _recv(int socket, void* buffer, size_t len, int flags);