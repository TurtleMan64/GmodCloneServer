#ifndef PLAYERCONNECTION_H
#define PLAYERCONNECTION_H

#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

#include "tcpclient.hpp"
#include "message.hpp"

class PlayerConnection
{
private:
    TcpClient* client = nullptr;
    std::thread* threadRead = nullptr;
    std::thread* threadWrite = nullptr;
    //std::vector<Message> messagesToSend;
    //std::mutex mutexMessages;
    char sendMsgBuf[188] = {0};

    //signals when a new message has showed up
    std::condition_variable condNewMessage;
    std::mutex mutexNewMessage;
    std::vector<Message> messagesToSend;

    static void behaviorRead(PlayerConnection* pc);

    static void behaviorWrite(PlayerConnection* pc);

public:
    bool running = true;
    std::string playerName = "error";
    Message disconnectMessage;

    PlayerConnection(TcpClient* client);

    void addMessage(Message msg);

    void flushMessages();

    void close();
};

#endif
