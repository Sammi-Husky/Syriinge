#include <string.h>

#include "ftp/ftp_session.h"
#include "ftp/ftp_utils.h"

FTPSession::FTPSession(int server)
{
    strncpy(m_cwd, "/", 2);
    m_ctrlSocket = -1;
    m_pasvSocket = -1;
    m_dataSocket = -1;
    m_restoffset = 0;

    struct sockaddr_in server_addr;
    u32 len = sizeof(server);
    m_ctrlSocket = accept(server, (struct sockaddr*)&server_addr, &len);

    // copy addr to pasv address
    len = sizeof(m_pasvAddr);
    getsockname(m_ctrlSocket, (struct sockaddr*)&m_pasvAddr, &len);

    m_dataAddr.sin_addr.s_addr = INADDR_ANY;

    // send ready response
    ftp_response(server, 220, "Welcome Aboard Captain\r\n");
}