#include <thread>
#include <csignal>
#include <vector>
#include <unordered_set>
#include <mutex>
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

std::string Global::pathToEXE;
std::unordered_set<Entity*> Global::gameEntities;
float dt;

std::mutex playerConnectionsMutex;
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
    Ball* ball1 = new Ball("B1", Vector3f(72.991318f, 28.784624f, -46.660568f), Vector3f(0, 0, 0)); INCR_NEW("Entity");
    Global::gameEntities.insert(ball1);

    glfwSetTime(0.0);

    Global::serverStartTime = Global::getRawUtcSystemTime();

    TcpListener* listener = new TcpListener(25567);
    listener->startListeneing();

    masterThread = new std::thread(masterServerLogic);

    while (isRunning)
    {
        TcpClient* client = listener->acceptTcpClient(-1);
        if (client != nullptr)
        {
            printf("Found new player: ");
            PlayerConnection* pc = new PlayerConnection(client);

            playerConnectionsMutex.lock();
            playerConnections.insert(pc);
            playerConnectionsMutex.unlock();
        }
        else if (!listener->isOpen())
        {
            isRunning = false;
            printf("listen had an error, closing\n");
        }
    }

    masterThread->join();
    delete masterThread;

    listener->close();
    delete listener;

    playerConnectionsMutex.lock();
    for (PlayerConnection* pc : playerConnections)
    {
        pc->close();
        delete pc;
    }
    playerConnections.clear();
    playerConnectionsMutex.unlock();
}

void broadcastMessage(Message msg, PlayerConnection* sender)
{
    playerConnectionsMutex.lock();

    for (PlayerConnection* pc : playerConnections)
    {
        if (pc != sender)
        {
            pc->addMessage(msg);
        }
    }

    playerConnectionsMutex.unlock();
}

void sendMessageToSpecificPlayer(Message msg, std::string playerName)
{
    playerConnectionsMutex.lock();

    for (PlayerConnection* pc : playerConnections)
    {
        if (pc->playerName == playerName)
        {
            pc->addMessage(msg);
            playerConnectionsMutex.unlock();
            return;
        }
    }

    playerConnectionsMutex.unlock();
    printf("Tried to send a message to '%s', but that player doesn't exist\n", playerName.c_str());
}

void masterServerLogic()
{
    double prevCheckDeadConnections = glfwGetTime();
    double prevStepUpdateTime = prevCheckDeadConnections;
    double prevTime = prevCheckDeadConnections;

    while (isRunning)
    {
        double currTime = glfwGetTime();
        double diff = currTime - prevTime;
        prevTime = currTime;

        dt = (float)diff;

        for (Entity* e : Global::gameEntities)
        {
            e->step();
        }

        if (currTime - prevStepUpdateTime >= 0.0166666f) //send entity updates every 16 milliseconds
        {
            //for (Entity* e : Global::gameEntities)
            {
                //e->step();
            }

            prevStepUpdateTime = glfwGetTime();
        }
        else
        {
            Sleep(1);
        }

        if (currTime - prevCheckDeadConnections >= 5.0) //Every 5 seconds, check for dead connections
        {
            std::vector<PlayerConnection*> pcsToDelete;

            playerConnectionsMutex.lock();

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

            for (PlayerConnection* pcToDelete : pcsToDelete)
            {
                playerConnections.erase(pcToDelete);
                printf("%s Disconnected\n", pcToDelete->playerName.c_str());
                pcToDelete->close();
                delete pcToDelete;
            }

            playerConnectionsMutex.unlock();

            prevCheckDeadConnections = glfwGetTime();
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
