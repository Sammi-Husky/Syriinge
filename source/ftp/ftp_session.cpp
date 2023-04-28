#include <VI/vi.h>
#include <string.h>

#include "debug.h"
#include "ftp/ftp_session.h"
#include "ftp/ftp_utils.h"

FTPSession::FTPSession(int server)
{
    strncpy(m_cwd, "/", 2);
    memset(m_buffer, 0, sizeof(m_buffer));
    m_dataAddr.sin_addr.s_addr = INADDR_ANY;
    m_ctrlSocket = -1;
    m_pasvSocket = -1;
    m_dataSocket = -1;
    m_restoffset = 0;

    struct sockaddr_in server_addr;
    u32 len = sizeof(server_addr);

    m_ctrlSocket = accept(server, (struct sockaddr*)&server_addr, &len);
    if (m_ctrlSocket < 0)
    {
        debug_log("Error accepting connection\n");
        return;
    }

    // copy addr to pasv address
    len = sizeof(m_pasvAddr);
    if (getsockname(m_ctrlSocket, (struct sockaddr*)&m_pasvAddr, &len) < 0)
    {
        debug_log("Failed to getsockname\n");
        return;
    }

    // send ready response
    ftp_response(m_ctrlSocket, 220, "Welcome Aboard Captain\r\n");
}
int FTPSession::OpenDataConnection()
{
    u32 addrlen = sizeof(m_dataAddr);
    m_dataSocket = accept(m_pasvSocket, (struct sockaddr*)&m_dataAddr, &addrlen);
    if (m_dataSocket < 0)
    {
        debug_log("Couldn't open data connection\n");
        return -1;
    }
    ftp_response(m_ctrlSocket, 150, "Data Connection Accepted\r\n");
    return 0;
}

void FTPSession::CloseDataConnection()
{
    closesocket(m_dataSocket);
    closesocket(m_pasvSocket);
    m_dataSocket = -1;
    m_pasvSocket = -1;
}

FTPSession::~FTPSession()
{
    closesocket(m_ctrlSocket);
    closesocket(m_dataSocket);
    closesocket(m_pasvSocket);
}