#include <thread>
#include <csignal>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <Windows.h>

#include <GLFW/glfw3.h>

#include "main.hpp"
#include "tcpclient.hpp"
#include "tcplistener.hpp"
#include "playerconnection.hpp"
#include "message.hpp"

#include "entities/entity.hpp"
#include "entities/ball.hpp"
#include "collision/collisionchecker.hpp"
#include "collision/collisionmodel.hpp"
#include "textures/modeltexture.hpp"
#include "loader/objloader.hpp"
#include "entities/healthcube.hpp"
#include "entities/glass.hpp"
#include "toolbox/levelloader.hpp"
#include "toolbox/maths.hpp"

std::string Global::pathToEXE;

std::shared_mutex Global::gameEntitiesSharedMutex;
std::shared_mutex Global::gameEntitiesMutex;
std::unordered_set<Entity*> Global::gameEntities;

int Global::levelId = 0;

float Global::timeUntilRoundStarts = 100000.0f;
float Global::timeUntilRoundEnds = 100000.0f;

Vector3f Global::safeZoneStart;
Vector3f Global::safeZoneEnd;

float dt;

std::shared_mutex playerConnectionsSharedMutex;
std::unordered_set<PlayerConnection*> playerConnections;

std::thread* masterThread = nullptr;

volatile bool isRunning = true;

void intHandler()
{
    isRunning = false;
}

void masterServerLogic();

int main(int argc, char** argv)
{
    if (argc > 0)
    {
        Global::pathToEXE = argv[0];

        #ifdef _WIN32
        int idx = (int)Global::pathToEXE.find_last_of('\\', Global::pathToEXE.size());
        Global::pathToEXE = Global::pathToEXE.substr(0, idx+1);
        #else
        int idx = (int)Global::pathToEXE.find_last_of('/', Global::pathToEXE.size());
        Global::pathToEXE = Global::pathToEXE.substr(0, idx+1);
        #endif
    }

    signal(SIGINT, (_crt_signal_t)intHandler);

    glfwInit();

    CollisionModel* cm = ObjLoader::loadCollisionModel("Models/TestMap/", "TestMap");
    for (int i = 0; i < cm->triangles.size(); i++)
    {
        CollisionChecker::addTriangle(cm->triangles[i]);
    }
    CollisionChecker::constructChunkDatastructure();

    LevelLoader::loadLevel("Map1");

    glfwSetTime(0.0);

    Global::serverStartTime = Global::getRawUtcSystemTime();

    TcpListener* listener = new TcpListener(25567); INCR_NEW("TcpListener");
    listener->startListeneing();

    masterThread = new std::thread(masterServerLogic); INCR_NEW("std::thread");

    while (isRunning)
    {
        TcpClient* client = listener->acceptTcpClient(-1);
        if (client != nullptr)
        {
            printf("Found new player: ");
            PlayerConnection* pc = new PlayerConnection(client); INCR_NEW("PlayerConnection");

            playerConnectionsSharedMutex.lock();
            playerConnections.insert(pc);
            playerConnectionsSharedMutex.unlock();
        }
        else if (!listener->isOpen())
        {
            isRunning = false;
            printf("listen had an error, closing\n");
        }
    }

    masterThread->join();
    delete masterThread; INCR_DEL("std::thread");

    listener->close();
    delete listener; INCR_DEL("TcpListener");

    playerConnectionsSharedMutex.lock();
    for (PlayerConnection* pc : playerConnections)
    {
        pc->close();
        delete pc; INCR_DEL("PlayerConnection");
    }
    playerConnections.clear();
    playerConnectionsSharedMutex.unlock();
}

void broadcastMessage(Message msg, PlayerConnection* sender)
{
    playerConnectionsSharedMutex.lock_shared();

    for (PlayerConnection* pc : playerConnections)
    {
        if (pc != sender)
        {
            pc->addMessage(msg);
        }
    }

    playerConnectionsSharedMutex.unlock_shared();
}

void sendMessageToSpecificPlayer(Message msg, std::string playerName)
{
    playerConnectionsSharedMutex.lock_shared();

    for (PlayerConnection* pc : playerConnections)
    {
        if (pc->playerName == playerName)
        {
            pc->addMessage(msg);
            playerConnectionsSharedMutex.unlock_shared();
            return;
        }
    }

    playerConnectionsSharedMutex.unlock_shared();
    printf("Tried to send a message to '%s', but that player doesn't exist\n", playerName.c_str());
}

std::shared_mutex debugMutex;
std::unordered_map<std::string, int> heapObjects;
int countNew = 0;
int countDelete = 0;

void masterServerLogic()
{
    double prevCheckDeadConnections = glfwGetTime();
    double prevCheckSafeZone = prevCheckDeadConnections;
    //double prevStepUpdateTime = prevCheckDeadConnections;
    double prevTime = prevCheckDeadConnections;

    while (isRunning)
    {
        double currTime = glfwGetTime();
        double diff = currTime - prevTime;
        prevTime = currTime;

        dt = (float)diff;

        float timeUntilRoundStartsBefore = Global::timeUntilRoundStarts;
        Global::timeUntilRoundStarts -= dt;

        // Go through and send updates to all objects that need updating before the round starts
        if (timeUntilRoundStartsBefore > 4.5f && Global::timeUntilRoundStarts <= 4.5f)
        {
            // Sync round clock
            Message timeMsg;
            timeMsg.length = 5;
            timeMsg.buf[0] = 7;
            memcpy(&timeMsg.buf[1], &Global::timeUntilRoundStarts, 4);

            broadcastMessage(timeMsg, nullptr);

            // Sync object data that needs syncing
            Global::gameEntitiesSharedMutex.lock_shared();
            for (Entity* e : Global::gameEntities)
            {
                switch (e->getEntityType())
                {
                    case ENTITY_GLASS:
                    {
                        Glass* glass = (Glass*)e;
            
                        int nameLen = (int)glass->name.size();
            
                        Message msg;
                        msg.length = 5 + nameLen + 2;
            
                        msg.buf[0] = 8;
                        memcpy(&msg.buf[1], &nameLen, 4);
                        memcpy(&msg.buf[5], glass->name.c_str(), nameLen);
                        msg.buf[5 + nameLen    ] = (char)glass->isReal;
                        msg.buf[5 + nameLen + 1] = (char)glass->hasBroken;
            
                        broadcastMessage(msg, nullptr);
                        break;
                    }
            
                    default: break;
                }
            }
            Global::gameEntitiesSharedMutex.unlock_shared();
        }

        float timeUntilRoundEndsBefore = Global::timeUntilRoundEnds;
        if (Global::timeUntilRoundStarts < 0.0f)
        {
            Global::timeUntilRoundEnds -= dt;
        }

        if (timeUntilRoundEndsBefore > -5.0f && Global::timeUntilRoundEnds <= -5.0f)
        {
            std::string lvlToLoad = "Map1";
            if (Maths::random() > 0.5f)
            {
                lvlToLoad = "Map2";
            }

            int lvlNameLen = (int)lvlToLoad.size();
            
            Message msg;
            msg.length = 5 + lvlNameLen;
            msg.buf[0] = 9;
            memcpy(&msg.buf[1], &lvlNameLen, 4);
            memcpy(&msg.buf[5], lvlToLoad.c_str(), lvlNameLen);
            
            broadcastMessage(msg, nullptr);
            
            LevelLoader::loadLevel(lvlToLoad);
        }


        //for (Entity* e : Global::gameEntities)
        {
            //e->step();
        }

        //if (currTime - prevStepUpdateTime >= 0.0166666f) //send entity updates every 16 milliseconds
        {
            //for (Entity* e : Global::gameEntities)
            {
                //e->step();
            }

            //prevStepUpdateTime = glfwGetTime();
        }
        //else
        {
            Sleep(1);
        }

         //Every 1 seconds, check if all playes have entered the safe zone
        if (currTime - prevCheckSafeZone >= 1.0)
        {
            if (Global::timeUntilRoundStarts < 0.0f && Global::timeUntilRoundEnds > 5.0f)
            {
                int totalAlivePlayers = 0;
                int totalAlivePlayersInSafeZone = 0;

                playerConnectionsSharedMutex.lock_shared();
                for (PlayerConnection* pc : playerConnections)
                {
                    if (pc->playerHealth > 0)
                    {
                        totalAlivePlayers++;
                    }
                    else
                    {
                        continue;
                    }

                    float x = pc->playerPos.x;
                    float y = pc->playerPos.y;
                    float z = pc->playerPos.z;

                    if (x > Global::safeZoneStart.x && x < Global::safeZoneEnd.x &&
                        y > Global::safeZoneStart.y && y < Global::safeZoneEnd.y &&
                        z > Global::safeZoneStart.z && z < Global::safeZoneEnd.z)
                    {
                        totalAlivePlayersInSafeZone++;
                    }
                }
                playerConnectionsSharedMutex.unlock_shared();

                bool endingTheRound = false;

                float inSafeZoneRatio = 0.0f;
                if (totalAlivePlayers > 0)
                {
                    inSafeZoneRatio = ((float)totalAlivePlayersInSafeZone)/totalAlivePlayers;
                }

                if (Global::levelId == LVL_MAP1 && inSafeZoneRatio >= 0.5f)
                {
                    endingTheRound = true;
                }
                else if (Global::levelId == LVL_MAP2 && inSafeZoneRatio >= 0.99f)
                {
                    endingTheRound = true;
                }

                // Enough players have finished, let's kill the rest and end the round
                if (endingTheRound)
                {
                    Global::timeUntilRoundEnds = 4.99f;

                    // Sync round clock
                    Message timeMsg;
                    timeMsg.length = 5;
                    timeMsg.buf[0] = 10;
                    memcpy(&timeMsg.buf[1], &Global::timeUntilRoundEnds, 4);

                    broadcastMessage(timeMsg, nullptr);
                }
            }

            prevCheckSafeZone = glfwGetTime();
        }

        if (currTime - prevCheckDeadConnections >= 5.0) //Every 5 seconds, check for dead connections
        {
            std::vector<PlayerConnection*> pcsToDelete;

            playerConnectionsSharedMutex.lock_shared();
            for (PlayerConnection* pc1 : playerConnections)
            {
                if (!pc1->running)
                {
                    pcsToDelete.push_back(pc1);

                    for (PlayerConnection* pc2 : playerConnections)
                    {
                        if (pc2 != pc1 && pc1->disconnectMessage.length > 1)
                        {
                            pc2->addMessage(pc1->disconnectMessage);
                        }
                    }
                }
            }
            playerConnectionsSharedMutex.unlock_shared();

            playerConnectionsSharedMutex.lock();
            for (PlayerConnection* pcToDelete : pcsToDelete)
            {
                playerConnections.erase(pcToDelete);
                printf("%s Disconnected\n", pcToDelete->playerName.c_str());
                pcToDelete->close();
                delete pcToDelete; INCR_DEL("PlayerConnection");
            }
            playerConnectionsSharedMutex.unlock();

            prevCheckDeadConnections = glfwGetTime();

            //std::fprintf(stdout, "diff: %d\n", countNew - countDelete);
            //std::unordered_map<std::string, int>::iterator it;
            //for (it = heapObjects.begin(); it != heapObjects.end(); it++)
            //{
            //    printf("%s: %d\n", it->first.c_str(), it->second);
            //}
            //printf("\n");
        }
    }
}

unsigned long long Global::serverStartTime; //when the server started up
unsigned long long Global::getRawUtcSystemTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    unsigned long high = (unsigned long)ft.dwHighDateTime;
    unsigned long low  = (unsigned long)ft.dwLowDateTime;

    unsigned long long totalT;
    memcpy(&totalT, &low, 4);
    memcpy(((char*)&totalT) + 4, &high, 4);

    return totalT;
}

void Global::debugNew(const char* name)
{
    debugMutex.lock();
    countNew++;

    #ifdef DEV_MODE
    if (heapObjects.find(name) == heapObjects.end())
    {
        heapObjects[name] = 1;
    }
    else
    {
        int num = heapObjects[name];
        heapObjects[name] = num+1;
    }
    #else
    name;
    #endif
    debugMutex.unlock();
}

void Global::debugDel(const char* name)
{
    debugMutex.lock();
    countDelete++;

    #ifdef DEV_MODE
    if (heapObjects.find(name) == heapObjects.end())
    {
        std::fprintf(stdout, "Warning: trying to delete '%s' when there are none.\n", name);
        heapObjects[name] = 0;
    }
    else
    {
        int num = heapObjects[name];
        heapObjects[name] = num-1;
    }
    #else
    name;
    #endif
    debugMutex.unlock();
}
