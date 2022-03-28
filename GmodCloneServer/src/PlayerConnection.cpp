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
#include "toolbox/maths.hpp"
#include "entities/bat.hpp"

#define TIMOUT 10

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

bool PlayerConnection::checkConnection(int numBytesExpected, int numBytesActual, std::string message, PlayerConnection* pc)
{
    if (!pc->client->isOpen() || numBytesActual != numBytesExpected)
    {
        printf("Error: Check connection failed because '%s'\n", message.c_str());
        printf("       Expected bytes: %d. Bytes actual: %d\n", numBytesExpected, numBytesActual);

        pc->mutexNewMessage.lock();
        pc->running = false;
        pc->condNewMessage.notify_all();
        pc->mutexNewMessage.unlock();

        return false;
    }
    
    return true;
}

void PlayerConnection::behaviorRead(PlayerConnection* pc)
{
    #define CHECK_CONNECTION_R(NUM_BYTES, MESSAGE) if (!checkConnection(NUM_BYTES, numRead, MESSAGE, pc)) { return; }

    while (pc->running)
    {
        char command;
        int numRead = pc->client->read(&command, 1, TIMOUT);
        if (numRead == 1)
        {
            switch (command)
            {
                case 0: //no op
                    printf("Warning: recieved no op command from %s\n", pc->playerName.c_str());
                    break;

                case 1: //time update
                {
                    //unsigned long long clientRawUtcTime;
                    double playerTime;
                    numRead = pc->client->read(&playerTime, 8, TIMOUT); CHECK_CONNECTION_R(8, "Could not read clientSyncedTime");

                    double ourTime = glfwGetTime();
                    int ping = (int)(((ourTime - playerTime)*1000));
                    if (ping < 0)
                    {
                        printf("ping was negative = %d\n (ourTime = %f playerTime = %f)\n", ping, ourTime, playerTime);
                        //ping = 0;
                    }
                    pc->pingMs = ping;
                    //printf("%s Ping = %dms\n", pc->playerName.c_str(), ping);

                    //ping = (int)(1000*Maths::random());

                    Message msg;
                    msg.length = 5;
                    msg.buf[0] = 13;
                    memcpy(&msg.buf[1], &ping, 4);
                    pc->addMessage(msg);

                    break;
                }

                case 2: //player update
                {
                    int nameLen = (int)pc->playerName.size();

                    Message msg;
                    msg.length = 5 + nameLen + 219 + 4;

                    // Copy the name over
                    memcpy(msg.buf, pc->sendMsgBuf, 5 + nameLen);

                    // Copy the rest of the data from the wire
                    numRead = pc->client->read(&msg.buf[5 + nameLen], 219, TIMOUT); CHECK_CONNECTION_R(219, "Could not read player update");

                    // Insert the players ping at the end of the message
                    memcpy(&msg.buf[5 + nameLen + 219], &pc->pingMs, 4);

                    // Send this player's status to all other players
                    broadcastMessage(msg, pc);

                    // Update various player vars that we need
                    memcpy(&pc->playerPos.x,      &msg.buf[5 + nameLen +  18], 4);
                    memcpy(&pc->playerPos.y,      &msg.buf[5 + nameLen +  22], 4);
                    memcpy(&pc->playerPos.z,      &msg.buf[5 + nameLen +  26], 4);
                    memcpy(&pc->playerWeapon,     &msg.buf[5 + nameLen + 142], 1);
                    memcpy(&pc->playerHealth,     &msg.buf[5 + nameLen + 143], 1);
                    memcpy(&pc->playerInZoneTime, &msg.buf[5 + nameLen + 144], 4);

                    // Go through and see if the player has picked up any items
                    if (pc->playerHealth > 0)
                    {
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
                                            int healthNameLen = (int)e->name.length();

                                            Message msgOut;
                                            msgOut.length = 5 + healthNameLen + 1;
                                            msgOut.buf[0] = 6;

                                            memcpy(&msgOut.buf[1], &healthNameLen, 4);
                                            memcpy(&msgOut.buf[5], e->name.c_str(), healthNameLen);

                                            // Send 25 health to the player that picked it up
                                            msgOut.buf[5 + healthNameLen] = 25;
                                            pc->addMessage(msgOut);

                                            // Send 0 health to everyone else (so that it will disappear in their game)
                                            // Need to make this a new message instead of reusing msgOut.. not really sure why.
                                            Message msgOut2;
                                            msgOut2.length = 5 + healthNameLen + 1;
                                            msgOut2.buf[0] = 6;
                                            memcpy(&msgOut2.buf[1], &healthNameLen, 4);
                                            memcpy(&msgOut2.buf[5], e->name.c_str(), healthNameLen);
                                            msgOut2.buf[5 + healthNameLen] = 0;
                                            broadcastMessage(msgOut2, pc);
                                        }
                                    }
                                    break;
                                }

                                case ENTITY_BAT:
                                {
                                    Vector3f playerPos = pc->playerPos;
                                    playerPos.y += COLLISION_RADIUS;
                                    Vector3f diff1 = playerPos - e->position;
                                    playerPos.y += 2*COLLISION_RADIUS;
                                    Vector3f diff2 = playerPos - e->position;

                                    Bat* bat = (Bat*)e;

                                    if (!bat->isCollected &&
                                        pc->playerWeapon == 0 &&
                                        (diff1.lengthSquared() < (COLLISION_RADIUS + 0.3f)*(COLLISION_RADIUS + 0.3f)) ||
                                        (diff2.lengthSquared() < (COLLISION_RADIUS + 0.3f)*(COLLISION_RADIUS + 0.3f)))
                                    {
                                        bool pickedUp = false;

                                        Global::gameEntitiesMutex.lock();
                                        if (!bat->isCollected)
                                        {
                                            bat->isCollected = true;
                                            pickedUp = true;
                                        }
                                        Global::gameEntitiesMutex.unlock();

                                        if (pickedUp)
                                        {
                                            int batNameLen = (int)e->name.length();

                                            Message msgOut;
                                            msgOut.length = 5 + batNameLen + 1;
                                            msgOut.buf[0] = 15;

                                            memcpy(&msgOut.buf[1], &batNameLen, 4);
                                            memcpy(&msgOut.buf[5], e->name.c_str(), batNameLen);

                                            // Send 1 to the player that picked it up
                                            msgOut.buf[5 + batNameLen] = 1;
                                            pc->addMessage(msgOut);

                                            // Send 0 to everyone else (so that it will disappear in their game)
                                            // Need to make this a new message instead of reusing msgOut.. not really sure why.
                                            Message msgOut2;
                                            msgOut2.length = 5 + batNameLen + 1;
                                            msgOut2.buf[0] = 15;
                                            memcpy(&msgOut2.buf[1], &batNameLen, 4);
                                            memcpy(&msgOut2.buf[5], e->name.c_str(), batNameLen);
                                            msgOut2.buf[5 + batNameLen] = 0;
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
                    }

                    break;
                }

                case 4: //player getting hit
                {
                    float x, y, z;
                    float dx, dy, dz;
                    char weapon;

                    char name[33] = {0};
                    int nameLen;

                    numRead = pc->client->read(&nameLen,   4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player name len");
                    numRead = pc->client->read(name, nameLen, TIMOUT); CHECK_CONNECTION_R(nameLen, "Could not read player name");
                    numRead = pc->client->read(&x,         4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player x");
                    numRead = pc->client->read(&y,         4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player y");
                    numRead = pc->client->read(&z,         4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player z");
                    numRead = pc->client->read(&dx,        4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player dx");
                    numRead = pc->client->read(&dy,        4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player dy");
                    numRead = pc->client->read(&dz,        4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read player dz");
                    numRead = pc->client->read(&weapon,    1, TIMOUT); CHECK_CONNECTION_R(1,       "Could not read player weapon");

                    std::string playerThatGotHit = name;

                    Message msg;
                    msg.length = 26;
                    msg.buf[0] = 4;
                    memcpy(&msg.buf[1 +  0], &x,      4);
                    memcpy(&msg.buf[1 +  4], &y,      4);
                    memcpy(&msg.buf[1 +  8], &z,      4);
                    memcpy(&msg.buf[1 + 12], &dx,     4);
                    memcpy(&msg.buf[1 + 16], &dy,     4);
                    memcpy(&msg.buf[1 + 20], &dz,     4);
                    memcpy(&msg.buf[1 + 24], &weapon, 1);

                    sendMessageToSpecificPlayer(msg, playerThatGotHit);
                    break;
                }

                case 5: //sending us a sound effect
                {
                    int sfxId;
                    Vector3f pos;

                    numRead = pc->client->read(&sfxId, 4, TIMOUT); CHECK_CONNECTION_R(4, "Could not read sfx id");
                    numRead = pc->client->read(&pos.x, 4, TIMOUT); CHECK_CONNECTION_R(4, "Could not read sfx x");
                    numRead = pc->client->read(&pos.y, 4, TIMOUT); CHECK_CONNECTION_R(4, "Could not read sfx y");
                    numRead = pc->client->read(&pos.z, 4, TIMOUT); CHECK_CONNECTION_R(4, "Could not read sfx z");

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

                    numRead = pc->client->read(&nameLen,        4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read glass plane name len");
                    numRead = pc->client->read(glassName, nameLen, TIMOUT); CHECK_CONNECTION_R(nameLen, "Could not read glass plane name");
                    numRead = pc->client->read(&isReal,         1, TIMOUT); CHECK_CONNECTION_R(1,       "Could not read glass plane isReal");
                    numRead = pc->client->read(&isBroken,       1, TIMOUT); CHECK_CONNECTION_R(1,       "Could not read glass plane isBroken");

                    Message msg;
                    msg.length = 5 + nameLen + 2;
                    msg.buf[0] = 8;
                    memcpy(&msg.buf[1],               &nameLen,        4);
                    memcpy(&msg.buf[5],               glassName, nameLen);
                    memcpy(&msg.buf[5 + nameLen    ], &isReal,         1);
                    memcpy(&msg.buf[5 + nameLen + 1], &isBroken,       1);

                    broadcastMessage(msg, pc);
                    break;
                }

                case 14: // Signal that a step fall platform has been stepped on.
                {
                    int nameLen;
                    char platName[33] = {0};

                    numRead = pc->client->read(&nameLen,       4, TIMOUT); CHECK_CONNECTION_R(4,       "Could not read step fall platform name len");
                    numRead = pc->client->read(platName, nameLen, TIMOUT); CHECK_CONNECTION_R(nameLen, "Could not read step fall platform name");

                    Message msg;
                    msg.length = 5 + nameLen;
                    msg.buf[0] = 14;
                    memcpy(&msg.buf[1], &nameLen, 4);
                    memcpy(&msg.buf[5], platName, nameLen);

                    broadcastMessage(msg, pc);
                    break;
                }

                case 15: // The player is disconnecting
                {
                    printf("%s left gracefully.\n", pc->playerName.c_str());
                    pc->mutexNewMessage.lock();
                    pc->running = false;
                    pc->condNewMessage.notify_all();
                    pc->mutexNewMessage.unlock();
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
            printf("Timeout when reading command from player %s\n", pc->playerName.c_str());
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
    #define CHECK_CONNECTION_W(NUM_BYTES, MESSAGE) if (!checkConnection(NUM_BYTES, numWritten, MESSAGE, pc)) { pc->condNewMessage.notify_all(); return; }

    // Write the initial server start time command
    char timeCmd = 1;
    int numWritten = pc->client->write(&timeCmd, 1, TIMOUT); CHECK_CONNECTION_W(1, "Could not write out time command player");

    double serverTime = glfwGetTime();
    numWritten = pc->client->write(&serverTime, 8, TIMOUT); CHECK_CONNECTION_W(8, "Could not write out time to player");

    double lastTimeSent = glfwGetTime();

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

        lock.unlock();

        if (glfwGetTime() - lastTimeSent >= 5.0)
        {
            numWritten = pc->client->write(&timeCmd, 1, TIMOUT); CHECK_CONNECTION_W(1, "Could not write out time command player");

            lastTimeSent = glfwGetTime();
            numWritten = pc->client->write(&lastTimeSent, 8, TIMOUT); CHECK_CONNECTION_W(8, "Could not write out time to player");
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
            printf("Didn't write enough bytes for command %d (Expcetd %d bytes, actual: %d)\n", msg.buf[0], msg.length, bytesWritten);
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
