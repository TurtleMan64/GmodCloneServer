#include <Windows.h>
#include <winsock.h>
#include <string>

#include "tcplistener.hpp"
#include "tcpclient.hpp"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

bool TcpListener::hasInit = false;

TcpListener::TcpListener(int port)
{
    if (!TcpListener::hasInit)
    {
        WSADATA wsaData;
        WSAStartup(0x0101, &wsaData);
        TcpListener::hasInit = true;
    }

    struct sockaddr_in sad; // structure to hold server's address

    memset((char*)&sad, 0, sizeof(sad)); // clear sockaddr structure
    sad.sin_family = AF_INET; // set family to Internet

    sad.sin_addr.s_addr = INADDR_ANY;
    sad.sin_port = htons((u_short)port);

    // Map TCP transport protocol name to protocol number
    struct protoent* ptrp = getprotobyname("tcp");
    if (ptrp == nullptr)
    {
        return;
    }

    // Create a socket
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd == INVALID_SOCKET)
    {
        return;
    }

    // Eliminate "Address already in use" error message.
    const char flag = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(char)) == -1)
    { 

    }

    // Bind a local address to the socket
    if (bind(sd, (struct sockaddr*)&sad, sizeof(sad)) < 0)
    {
        closesocket(sd);
        return;
    }
}

TcpListener::~TcpListener()
{
    if (sd != INVALID_SOCKET)
    {
        closesocket(sd);
        sd = INVALID_SOCKET;
    }
}

int TcpListener::startListeneing()
{
    if (listen(sd, SOMAXCONN) < 0)
    {
        closesocket(sd);
        return -1;
    }

    return 0;
}

TcpClient* TcpListener::acceptTcpClient(int timeoutSec)
{
    if (timeoutSec >= 0)
    {
        fd_set socketSet = {0};
        struct timeval timeout = {0};
        FD_ZERO(&socketSet);
        FD_SET(sd, &socketSet);
        timeout.tv_sec = timeoutSec;
        timeout.tv_usec = 0;

        int selectval = select((int)sd + 1, &socketSet, nullptr, nullptr, &timeout);
        if (selectval == -1)
        {
            return nullptr;
        }
        else if (selectval == 0) // Timeout happened
        {
            return nullptr;
        }

        // Ready to call accept without blocking
    }

    struct sockaddr_in cad; // structure to hold server's address
    int alen = (int)sizeof(cad);
    memset((char*)&cad, 0, sizeof(cad)); // clear sockaddr structure

    SOCKET socketClient = accept(sd, (struct sockaddr*)(&(cad)), &alen);
    if (socketClient < 0)
    {
        return nullptr;
    }

    return new TcpClient(socketClient);
}

bool TcpListener::isOpen()
{
    return (sd != INVALID_SOCKET);
}

void TcpListener::close()
{
    if (sd != INVALID_SOCKET)
    {
        closesocket(sd);
        sd = INVALID_SOCKET;
    }
}
