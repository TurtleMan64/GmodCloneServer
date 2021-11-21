#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <Windows.h>

#include <GLFW/glfw3.h>

#include "playerconnection.hpp"
#include "message.hpp"
#include "toolbox/vector.hpp"
#include "main.hpp"

#define TIMOUT 50

extern void broadcastMessage(Message msg, PlayerConnection* sender);

extern void sendMessageToSpecificPlayer(Message msg, std::string playerName);

PlayerConnection::PlayerConnection(TcpClient* client)
{
    this->client = client;

    disconnectMessage.length = 1;
    disconnectMessage.buf[0] = 0;//no op message as default disconnect

    int nameLen;
    int read = client->read((char*)&nameLen, 4, TIMOUT);
    if (read != 4)
    {
        printf("Could not read player name length\n");
        running = false;
        return;
    }
    else if (nameLen > 32)
    {
        printf("Someone tried connecting with a long name (%d characters)\n", nameLen);
        running = false;
        return;
    }
    else
    {
        char name[33];
        for (int i = 0; i < 33; i++)
        {
            name[i] = 0;
        }

        read = client->read(name, nameLen, TIMOUT);

        if (read != nameLen)
        {
            printf("Could not read player name\n");
            running = false;
            return;
        }

        playerName = name;

        printf("%s\n", name);
    }

    sendMsgBuf[0] = 2;
    memcpy(&sendMsgBuf[1], &nameLen, 4);
    for (int i = 0; i < nameLen; i++)
    {
        sendMsgBuf[i + 5] = playerName[i];
    }

    disconnectMessage.length = nameLen + 5;
    disconnectMessage.buf[0] = 3; //Player disconnects
    memcpy(&disconnectMessage.buf[1], &nameLen, 4);
    for (int i = 0; i < nameLen; i++)
    {
        disconnectMessage.buf[i + 5] = playerName[i];
    }

    threadRead  = new std::thread(PlayerConnection::behaviorRead,  this);
    threadWrite = new std::thread(PlayerConnection::behaviorWrite, this);
}

void PlayerConnection::addMessage(Message msg)
{
    mutex.lock();

    messagesToSend.push_back(msg);

    mutex.unlock();
}

void PlayerConnection::flushMessages()
{
    mutex.lock();

    for (Message msg : messagesToSend)
    {
        int bytesWritten = client->write(msg.buf, msg.length, TIMOUT);
        if (bytesWritten != msg.length)
        {
            printf("Didn't write enough bytes.\n");
            running = false;
            mutex.unlock();
            return;
        }
    }

    messagesToSend.clear();

    mutex.unlock();
}

void PlayerConnection::behaviorRead(PlayerConnection* pc)
{
    while (pc->running)
    {
        char command;
        int numRead = pc->client->read(&command, 1, TIMOUT);
        if (numRead == 1)
        {
            switch (command)
            {
                case 0: //no op
                    break;

                case 1: //time update
                {
                    unsigned long long clientRawUtcTime;
                    numRead = pc->client->read((char*)&clientRawUtcTime, 8, TIMOUT);
                    if (numRead != 8)
                    {
                        printf("Could not read clientSyncedTime\n");
                        pc->running = false;
                        return;
                    }

                    int ping = (int)((Global::getRawUtcSystemTime() - clientRawUtcTime)/10000);
                    if (ping < 0)
                    {
                        ping = 0;
                    }
                    printf("%s Ping = %dms\n", pc->playerName.c_str(), ping);

                    break;
                }

                case 2: //player update
                {
                    int nameLen = (int)pc->playerName.size();

                    Message msg;
                    msg.length = 5 + nameLen + 144;

                    // Copy the name over
                    memcpy(msg.buf, pc->sendMsgBuf, 5 + nameLen);

                    // Copy the rest of the data from the wire
                    pc->client->read(&msg.buf[5 + nameLen], 144, 5);

                    if (!pc->client->isOpen())
                    {
                        pc->running = false;
                        break;
                    }

                    broadcastMessage(msg, pc);

                    break;
                }

                case 4: //player getting hit
                {
                    float x, y, z;
                    float dx, dy, dz;
                    char weapon;

                    char name[33] = {0};
                    int nameLen;

                    pc->client->read((char*)&nameLen, 4, 5);
                    pc->client->read(name, nameLen, 5);
                    pc->client->read((char*)&x,  4, 5);
                    pc->client->read((char*)&y,  4, 5);
                    pc->client->read((char*)&z,  4, 5);
                    pc->client->read((char*)&dx, 4, 5);
                    pc->client->read((char*)&dy, 4, 5);
                    pc->client->read((char*)&dz, 4, 5);
                    pc->client->read((char*)&weapon, 1, 5);

                    if (!pc->client->isOpen())
                    {
                        pc->running = false;
                        break;
                    }

                    std::string playerThatGotHit = name;

                    Message msg;
                    msg.length = 26;
                    msg.buf[0] = 4;
                    memcpy(&msg.buf[1 +  0], &x,  4);
                    memcpy(&msg.buf[1 +  4], &y,  4);
                    memcpy(&msg.buf[1 +  8], &z,  4);
                    memcpy(&msg.buf[1 + 12], &dx, 4);
                    memcpy(&msg.buf[1 + 16], &dy, 4);
                    memcpy(&msg.buf[1 + 20], &dz, 4);
                    memcpy(&msg.buf[1 + 24], &weapon, 1);

                    sendMessageToSpecificPlayer(msg, playerThatGotHit);
                    break;
                }

                case 5: //sending us a sound effect
                {
                    int sfxId;
                    Vector3f pos;

                    pc->client->read((char*)&sfxId, 4, 5);
                    pc->client->read((char*)&pos.x, 4, 5);
                    pc->client->read((char*)&pos.y, 4, 5);
                    pc->client->read((char*)&pos.z, 4, 5);

                    if (!pc->client->isOpen())
                    {
                        pc->running = false;
                        break;
                    }

                    Message msg;
                    msg.length = 1 + 4 + 12;
                    msg.buf[0] = 5;
                    memcpy(&msg.buf[1 +  0], &sfxId, 4);
                    memcpy(&msg.buf[1 +  4], &pos.x, 4);
                    memcpy(&msg.buf[1 +  8], &pos.y, 4);
                    memcpy(&msg.buf[1 + 12], &pos.z, 4);

                    broadcastMessage(msg, pc);
                    break;
                }

                default:
                    break;
            }
        }
        else if (numRead == 0)
        {
            //timeout, will happen often probably
            //Sleep(1);
        }
        else
        {
            printf("Could not read command from player\n");
            pc->running = false;
            return;
        }
    }
}

void PlayerConnection::behaviorWrite(PlayerConnection* pc)
{
    // Write the initial server start time command
    char cmd = 1;
    int numWrite = pc->client->write(&cmd, 1, TIMOUT);
    if (numWrite != 1)
    {
        printf("Could not write out time command player\n");
        pc->running = false;
        return;
    }

    numWrite = pc->client->write((char*)&Global::serverStartTime, 8, TIMOUT);
    if (numWrite != 8)
    {
        printf("Could not write out time to player\n");
        pc->running = false;
        return;
    }

    while (pc->running)
    {
        if (pc->messagesToSend.size() > 0)
        {
            pc->flushMessages();
        }
        else
        {
            Sleep(1);
        }
    }
}

void PlayerConnection::close()
{
    running = false;

    if (threadRead != nullptr)
    {
        threadRead->join();
        delete threadRead;
    }

    if (threadWrite != nullptr)
    {
        threadWrite->join();
        delete threadWrite;
    }

    client->close();
    delete client;
}
