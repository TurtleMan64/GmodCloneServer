#ifndef TCPLISTENER_H
#define TCPLISTENER_H

class TcpClient;

#include <winsock.h>
#include <string>

class TcpListener
{
private:
    static bool hasInit;

    SOCKET sd = INVALID_SOCKET;

public:
    TcpListener(int port);
    ~TcpListener();

    int startListeneing();

    // if timeoutSec < 0, block forever until connection
    TcpClient* acceptTcpClient(int timeoutSec);

    bool isOpen();

    void close();
};

#endif
