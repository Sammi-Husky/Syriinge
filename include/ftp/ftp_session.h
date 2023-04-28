#pragma once

#include <net/net.h>

#include "ftp/ftp.h"

class FTPSession {
public:
    char m_cwd[PATH_BUF_SIZE];
    char m_buffer[PATH_BUF_SIZE]; // working buffer
    int m_ctrlSocket;             // cmd control socket
    int m_pasvSocket;             // socket to listen for pasv connections
    int m_dataSocket;             // socket for data transfer

    struct sockaddr_in m_dataAddr; // addr for data connection
    struct sockaddr_in m_pasvAddr; // addr for pasv connection

    FTP_DATA_MODE m_dataMode; // ASCII or IMAGE (binary)
    FTP_XFER_MODE m_xferMode; // PASV or PORT

    int m_restoffset; // offset to resume transfers at after rest

    FTPSession(int server);
    ~FTPSession();
    int OpenDataConnection();
    void CloseDataConnection();
};
