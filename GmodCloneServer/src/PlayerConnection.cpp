#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <Windows.h>

#include <GLFW/glfw3.h>

#include "playerconnection.hpp"
#include "message.hpp"
#include "toolbox/vector.hpp"
#include "main.hpp"
#include "entities/entity.hpp"
#include "entities/healthcube.hpp"
#include "entities/ball.hpp"

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

    threadRead  = new std::thread(PlayerConnection::behaviorRead,  this); INCR_NEW("std::thread");
    threadWrite = new std::thread(PlayerConnection::behaviorWrite, this); INCR_NEW("std::thread");
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
                        pc->mutexNewMessage.lock();
                        pc->running = false;
                        pc->condNewMessage.notify_all();
                        pc->mutexNewMessage.unlock();
                        return;
                    }

                    int ping = (int)((Global::getRawUtcSystemTime() - clientRawUtcTime)/10000);
                    if (ping < 0)
                    {
                        ping = 0;
                    }
                    pc->pingMs = ping;
                    //printf("%s Ping = %dms\n", pc->playerName.c_str(), ping);

                    break;
                }

                case 2: //player update
                {
                    int nameLen = (int)pc->playerName.size();

                    Message msg;
                    msg.length = 5 + nameLen + 144 + 4;

                    // Copy the name over
                    memcpy(msg.buf, pc->sendMsgBuf, 5 + nameLen);

                    // Copy the rest of the data from the wire
                    pc->client->read(&msg.buf[5 + nameLen], 144, 5);

                    memcpy(&msg.buf[5 + nameLen + 144], &pc->pingMs, 4);

                    if (!pc->client->isOpen())
                    {
                        pc->mutexNewMessage.lock();
                        pc->running = false;
                        pc->condNewMessage.notify_all();
                        pc->mutexNewMessage.unlock();
                        break;
                    }

                    // Send this player's status to all other players
                    broadcastMessage(msg, pc);

                    // Update the player position and health
                    memcpy((char*)&pc->playerPos.x, &msg.buf[5 + nameLen + 18], 4);
                    memcpy((char*)&pc->playerPos.y, &msg.buf[5 + nameLen + 22], 4);
                    memcpy((char*)&pc->playerPos.z, &msg.buf[5 + nameLen + 26], 4);
                    memcpy(&pc->playerHealth, &msg.buf[5 + nameLen + 143], 1);

                    // Go through and see if the player has picked up any items

                    const float COLLISION_RADIUS = 1.74f/4.0f;

                    Global::gameEntitiesSharedMutex.lock_shared();
                    for (Entity* e : Global::gameEntities)
                    {
                        switch (e->getEntityType())
                        {
                            case ENTITY_HEALTH_CUBE:
                            {
                                Vector3f playerPos = pc->playerPos;
                                playerPos.y += COLLISION_RADIUS;
                                Vector3f diff1 = playerPos - e->position;
                                playerPos.y += 2*COLLISION_RADIUS;
                                Vector3f diff2 = playerPos - e->position;

                                HealthCube* healthCube = (HealthCube*)e;

                                if (!healthCube->isCollected &&
                                    pc->playerHealth < 100 &&
                                    (diff1.lengthSquared() < (COLLISION_RADIUS + 0.3f)*(COLLISION_RADIUS + 0.3f)) ||
                                    (diff2.lengthSquared() < (COLLISION_RADIUS + 0.3f)*(COLLISION_RADIUS + 0.3f)))
                                {
                                    bool pickedUp = false;

                                    Global::gameEntitiesMutex.lock();
                                    if (!healthCube->isCollected)
                                    {
                                        healthCube->isCollected = true;
                                        pickedUp = true;
                                    }
                                    Global::gameEntitiesMutex.unlock();

                                    if (pickedUp)
                                    {
                                        Message msgOut;
                                        msgOut.length = 5 + nameLen + 1;
                                        msgOut.buf[0] = 6;

                                        int healthNameLen = (int)e->name.length();
                                        memcpy(&msgOut.buf[1], &healthNameLen, 4);
                                        memcpy(&msgOut.buf[5], e->name.c_str(), healthNameLen);

                                        // Send 25 health to the player that picked it up
                                        msgOut.buf[5 + healthNameLen] = 25;
                                        pc->addMessage(msgOut);

                                        // Send 0 health to everyone else (so that it will disappear in their game)
                                        // Need to make this a new message instead of reusing msgOut.. not really sure why.
                                        Message msgOut2;
                                        msgOut2.length = 5 + nameLen + 1;
                                        msgOut2.buf[0] = 6;
                                        memcpy(&msgOut2.buf[1], &healthNameLen, 4);
                                        memcpy(&msgOut2.buf[5], e->name.c_str(), healthNameLen);
                                        msgOut2.buf[5 + healthNameLen] = 0;
                                        broadcastMessage(msgOut2, pc);
                                    }
                                }
                                break;
                            }

                            default:
                                break;
                        }
                    }
                    Global::gameEntitiesSharedMutex.unlock_shared();

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
                        pc->mutexNewMessage.lock();
                        pc->running = false;
                        pc->condNewMessage.notify_all();
                        pc->mutexNewMessage.unlock();
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
                        pc->mutexNewMessage.lock();
                        pc->running = false;
                        pc->condNewMessage.notify_all();
                        pc->mutexNewMessage.unlock();
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

                case 8: // Updating status of glass plane
                {
                    int nameLen;
                    char glassName[33] = {0};
                    char isReal;
                    char isBroken;

                    pc->client->read((char*)&nameLen, 4, 5);
                    pc->client->read(glassName, nameLen, 5);
                    pc->client->read(&isReal,   1, 5);
                    pc->client->read(&isBroken, 1, 5);

                    Message msg;
                    msg.length = 5 + nameLen + 2;
                    msg.buf[0] = 8;
                    memcpy(&msg.buf[1], &nameLen, 4);
                    memcpy(&msg.buf[5], glassName, nameLen);
                    memcpy(&msg.buf[5 + nameLen    ], &isReal, 1);
                    memcpy(&msg.buf[5 + nameLen + 1], &isBroken, 1);

                    if (!pc->client->isOpen())
                    {
                        pc->mutexNewMessage.lock();
                        pc->running = false;
                        pc->condNewMessage.notify_all();
                        pc->mutexNewMessage.unlock();
                        break;
                    }

                    broadcastMessage(msg, pc);
                    break;
                }

                default:
                {
                    printf("Error: Received unknown command %d from player %s\n", command, pc->playerName.c_str());
                    break;
                }
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
            pc->mutexNewMessage.lock();
            pc->running = false;
            pc->condNewMessage.notify_all();
            pc->mutexNewMessage.unlock();
            return;
        }
    }

    pc->condNewMessage.notify_all();
}

void PlayerConnection::behaviorWrite(PlayerConnection* pc)
{
    // Write the initial server start time command
    char cmd = 1;
    int numWrite = pc->client->write(&cmd, 1, TIMOUT);
    if (numWrite != 1)
    {
        printf("Could not write out time command player\n");
        pc->mutexNewMessage.lock();
        pc->running = false;
        pc->condNewMessage.notify_all();
        pc->mutexNewMessage.unlock();
        return;
    }

    numWrite = pc->client->write((char*)&Global::serverStartTime, 8, TIMOUT);
    if (numWrite != 8)
    {
        printf("Could not write out time to player\n");
        pc->mutexNewMessage.lock();
        pc->running = false;
        pc->condNewMessage.notify_all();
        pc->mutexNewMessage.unlock();
        return;
    }

    while (pc->running)
    {
        std::unique_lock<std::mutex> lock{pc->mutexNewMessage};
        pc->condNewMessage.wait(lock, [&]()
        {
            // Acquire the lock only if
            // we've stopped or the queue
            // isn't empty
            return !pc->running || pc->messagesToSend.size() > 0;
        });

        if (!pc->running)
        {
            break;
        }

        // We own the mutex here; pop the queue
        // until it empties out.
        if (pc->messagesToSend.size() > 0)
        {
            pc->flushMessages();
        }
    }

    pc->condNewMessage.notify_all();
}

void PlayerConnection::addMessage(Message msg)
{
    // Always lock before changing
    // state guarded by a mutex and
    // condition_variable (a.k.a. "condvar").
    mutexNewMessage.lock();

    messagesToSend.push_back(msg);

    // Tell the consumer it has a new message
    condNewMessage.notify_one();

    mutexNewMessage.unlock();
}

void PlayerConnection::flushMessages()
{
    for (Message msg : messagesToSend)
    {
        int bytesWritten = client->write(msg.buf, msg.length, TIMOUT);
        if (bytesWritten != msg.length)
        {
            printf("Didn't write enough bytes.\n");
            running = false;
            condNewMessage.notify_all();
            break;
        }
    }

    messagesToSend.clear();
}

void PlayerConnection::close()
{
    mutexNewMessage.lock();
    running = false;
    condNewMessage.notify_all();
    mutexNewMessage.unlock();

    if (threadRead != nullptr)
    {
        threadRead->join();
        delete threadRead; INCR_DEL("std::thread");
    }

    if (threadWrite != nullptr)
    {
        threadWrite->join();
        delete threadWrite; INCR_DEL("std::thread");
    }

    client->close();
    delete client; INCR_DEL("TcpClient");
}
