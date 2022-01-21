#ifndef PLAYERCONNECTION_H
#define PLAYERCONNECTION_H

#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

#include "tcpclient.hpp"
#include "message.hpp"
#include "toolbox/vector.hpp"

class PlayerConnection
{
private:
    TcpClient* client = nullptr;
    std::thread* threadRead = nullptr;
    std::thread* threadWrite = nullptr;
    //std::vector<Message> messagesToSend;
    //std::mutex mutexMessages;
    char sendMsgBuf[250] = {0};

    //signals when a new message has showed up
    std::condition_variable condNewMessage;
    std::mutex mutexNewMessage;
    std::vector<Message> messagesToSend;

    // Returns false if the connection was closed or didn't read enough bytes.
    static bool checkConnection(int numBytesExpected, int bytesRead, std::string message, PlayerConnection* pc);

    static void behaviorRead(PlayerConnection* pc);

    static void behaviorWrite(PlayerConnection* pc);

public:
    bool running = true;
    std::string playerName = "error";
    Vector3f playerPos;
    char playerHealth = 100;
    char playerWeapon = 0;
    int pingMs = 0;
    float playerInZoneTime = 0.0f;
    Message disconnectMessage;

    PlayerConnection(TcpClient* client);

    void addMessage(Message msg);

    void flushMessages();

    void close();
};

#endif
