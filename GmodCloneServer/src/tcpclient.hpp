#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <winsock.h>
#include <string>

class TcpClient
{
private:
    static bool hasInit;

    SOCKET sd = INVALID_SOCKET;

    void attemptConnection(char* ip, int port, int timeoutSec);

public:
    TcpClient(SOCKET socket);
    TcpClient(char* ip, int port, int timeoutSec = -1);
    TcpClient(const char* ip, int port, int timeoutSec = -1);
    ~TcpClient();

    // Returns number of bytes written. If this is not equal to numBytesToSend, 
    // this means some error has happened and the connection has been closed.
    int write(void* bytes, int numBytesToSend, int timeoutSec = -1);

    // Returns number of bytes read.
    // If timeout happens, returns 0.
    // If error happens, closes connection and returns -1.
    // If client has closed connection, closes connection and returns -2.
    // If bad parameters, returns -3.
    int read(void* buffer, int numBytesToRead, int timeoutSec = -1);

    bool isOpen();

    void close();
};

#endif
