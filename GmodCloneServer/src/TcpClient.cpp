#include <Windows.h>
#include <winsock.h>
#include <string>

#include "tcpclient.hpp"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

bool TcpClient::hasInit = false;

TcpClient::TcpClient(SOCKET socket)
{
    if (!TcpClient::hasInit)
    {
        WSADATA wsaData;
        WSAStartup(0x0101, &wsaData);
        TcpClient::hasInit = true;
    }

    sd = socket;
}

TcpClient::TcpClient(char* ip, int port, int timeoutSec)
{
    if (!TcpClient::hasInit)
    {
        WSADATA wsaData;
        WSAStartup(0x0101, &wsaData);
        TcpClient::hasInit = true;
    }

    attemptConnection(ip, port, timeoutSec);
}

TcpClient::TcpClient(const char* ip, int port, int timeoutSec)
{
    if (!TcpClient::hasInit)
    {
        WSADATA wsaData;
        WSAStartup(0x0101, &wsaData);
        TcpClient::hasInit = true;
    }

    attemptConnection((char*)ip, port, timeoutSec);
}

TcpClient::~TcpClient()
{
    if (sd != INVALID_SOCKET)
    {
        closesocket(sd);
        sd = INVALID_SOCKET;
    }
}

void TcpClient::attemptConnection(char* ip, int port, int timeoutSec)
{
    if (sd != INVALID_SOCKET)
    {
        return;
    }

    struct sockaddr_in sad; // structure to hold server's address

    memset((char*)&sad, 0, sizeof(sad)); // clear sockaddr structure
    sad.sin_family = AF_INET; // set family to Internet

    sad.sin_addr.s_addr = inet_addr(ip);
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

    // If the timeout is < 0, block until connection succeeds or error
    if (timeoutSec < 0)
    {
        if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0)
        {
            closesocket(sd);
            sd = INVALID_SOCKET;
            return;
        }

        return;
    }

    // Set the socket in non-blocking
    unsigned long iMode = 1;
    int iResult = ioctlsocket(sd, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {	
        closesocket(sd);
        sd = INVALID_SOCKET;
        return;
    }

    connect(sd, (struct sockaddr *)&sad, sizeof(sad));

    // Put socket back into blocking mode
    iMode = 0;
    iResult = ioctlsocket(sd, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {	

    }

    fd_set setWrite;
    FD_ZERO(&setWrite);
    FD_SET(sd, &setWrite);

    timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = 0;

    // Wait until socket can be written to, or until timeout
    select(0, nullptr, &setWrite, nullptr, &timeout);
    if (FD_ISSET(sd, &setWrite))
    {

    }
    else
    {
        closesocket(sd);
        sd = INVALID_SOCKET;
    }
}

bool TcpClient::isOpen()
{
    return (sd != INVALID_SOCKET);
}

int TcpClient::write(char* bytes, int numBytesToSend, int timeoutSec)
{
    if (sd == INVALID_SOCKET ||
        numBytesToSend <= 0  ||
        bytes == nullptr)
    {
        return -1;
    }

    int currentBytesSent = 0;
    while (currentBytesSent < numBytesToSend)
    {
        if (timeoutSec > 0)
        {
            fd_set setWrite = {0};
            struct timeval timeout = {0};
            FD_ZERO(&setWrite);
            FD_SET(sd, &setWrite);

            timeout.tv_sec = timeoutSec;
            timeout.tv_usec = 0;
            int selectval = select((int)sd + 1, NULL, &setWrite, NULL, &timeout);

            if (selectval == -1)
            {
                closesocket(sd);
                sd = INVALID_SOCKET;
                return currentBytesSent;
            }
            else if (selectval == 0) // Timeout happened
            {
                closesocket(sd);
                sd = INVALID_SOCKET;
                return currentBytesSent;
            }
        }

        // Send response
        int flags = 0; //| MSG_DONTWAIT; //not waiting (MSG_DONTWAIT) causes the return to happen early and not send all the bytes much more often.

        int numBytesSent = send(sd, &bytes[currentBytesSent], numBytesToSend - currentBytesSent, flags);

        if (numBytesSent == -1)
        {
            closesocket(sd);
            sd = INVALID_SOCKET;
            return currentBytesSent;
        }
        else if (numBytesSent == 0)
        {
            closesocket(sd);
            sd = INVALID_SOCKET;
            return currentBytesSent;
        }

        currentBytesSent += numBytesSent;
    }

    return currentBytesSent;
}

int TcpClient::read(char* buffer, int numBytesToRead, int timeoutSec)
{
    if (sd == INVALID_SOCKET ||
        numBytesToRead <= 0  ||
        buffer == nullptr)
    {
        return -3;
    }

    int currentBytesRead = 0;
    while (currentBytesRead < numBytesToRead)
    {
        if (timeoutSec > 0)
        {
            fd_set setRead = {0};
            struct timeval timeout = {0};
            FD_ZERO(&setRead);
            FD_SET(sd, &setRead);
            timeout.tv_sec = timeoutSec;
            timeout.tv_usec = 0;
            int selectval = select((int)sd + 1, &setRead, NULL, NULL, &timeout);
            if (selectval == -1)
            {
                closesocket(sd);
                sd = INVALID_SOCKET;
                return -1;
            }
            else if (selectval == 0) // Timeout happened
            {
                return 0;
            }
        }

        int flags = 0;

        int numBytesRead = recv(sd, &buffer[currentBytesRead], numBytesToRead - currentBytesRead, flags);

        if (numBytesRead == -1)
        {
            closesocket(sd);
            sd = INVALID_SOCKET;
            return -1;
        }

        if (numBytesRead == 0)
        {
            closesocket(sd);
            sd = INVALID_SOCKET;
            return -2;
        }

        currentBytesRead += numBytesRead;
    }

    return currentBytesRead;
}

void TcpClient::close()
{
    if (sd != INVALID_SOCKET)
    {
        closesocket(sd);
        sd = INVALID_SOCKET;
    }
}
