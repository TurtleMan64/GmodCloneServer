#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <list>
#include <windows.h>

#include <chrono>
#include <thread>
#include <GLFW/glfw3.h>

#include "levelloader.hpp"
#include "../main.hpp"
#include "../collision/collisionchecker.hpp"
#include "../collision/collisionmodel.hpp"
#include "../toolbox/getline.hpp"
#include "../toolbox/maths.hpp"
#include "../toolbox/split.hpp"
#include "../entities/ball.hpp"
#include "../entities/entity.hpp"
#include "../entities/glass.hpp"
#include "../entities/healthcube.hpp"
#include "../loader/objloader.hpp"
#include "../entities/rockplatform.hpp"
#include "../message.hpp"
#include "../playerconnection.hpp"

void LevelLoader::loadLevel(std::string mapName)
{
    //convert name to lowercase name
    char nameLower[32] = {0};
    if ((int)mapName.size() < 32)
    {
        for (int i = 0; i < (int)mapName.size(); i++)
        {
            char c = mapName[i];

            if (c >= 65 && c <= 90)
            {
                c += 32;
            }

            nameLower[i] = c;
        }
    }

    std::string lower = nameLower;
    std::string fname = lower + ".map";

    std::ifstream file(Global::pathToEXE + "res/Maps/" + fname);
    if (!file.is_open())
    {
        printf("Error: Cannot load file '%s'\n", (Global::pathToEXE + "res/Maps/" + fname).c_str());
        file.close();
        return;
    }

    Global::gameEntitiesSharedMutex.lock();
    for (Entity* e : Global::gameEntities)
    {
        delete e; INCR_DEL("Entity");
    }
    Global::gameEntities.clear();
    Global::gameEntitiesSharedMutex.unlock();

    Global::timeUntilRoundStarts = 7.0f;

    std::chrono::high_resolution_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
    bool waitForSomeTime = true;

    if      (fname == "hub.map")  Global::levelId = LVL_HUB;
    else if (fname == "map1.map") Global::levelId = LVL_MAP1;
    else if (fname == "map2.map") Global::levelId = LVL_MAP2;
    else if (fname == "map3.map") Global::levelId = LVL_MAP3;
    else if (fname == "eq.map")   Global::levelId = LVL_EQ;
    else if (fname == "map4.map") Global::levelId = LVL_MAP4;
    else if (fname == "test.map") Global::levelId = LVL_TEST;
    else if (fname == "map5.map") Global::levelId = LVL_MAP5;

    //Run through the header content

    std::string modelVisualLine;
    getlineSafe(file, modelVisualLine);
    //std::vector<std::string> visualModel = split(modelVisualLine, ' ');

    std::string modelCollisionLine;
    getlineSafe(file, modelCollisionLine);
    //std::vector<std::string> collisionModel = split(modelCollisionLine, ' ');
    //CollisionChecker::deleteAllTriangles();
    //CollisionModel* cm = ObjLoader::loadCollisionModel(collisionModel[0], collisionModel[1]);
    //for (int i = 0; i < cm->triangles.size(); i++)
    //{
        //CollisionChecker::addTriangle(cm->triangles[i]);
    //}
    //CollisionChecker::constructChunkDatastructure();
    //delete cm; INCR_DEL("CollisionModel");

    std::string playerSpawnZoneLine;
    getlineSafe(file, playerSpawnZoneLine);
    //std::vector<std::string> spawnZones = split(playerSpawnZoneLine, ' ');
    //Vector3f spawnStart = Vector3f(toF(spawnZones[0]), toF(spawnZones[1]), toF(spawnZones[2]));
    //Vector3f spawnEnd   = Vector3f(toF(spawnZones[3]), toF(spawnZones[4]), toF(spawnZones[5]));
    //Vector3f spawnZone  = spawnEnd - spawnStart;
    //Global::player->position = spawnStart;
    //Global::player->position.x += Maths::random()*spawnZone.x;
    //Global::player->position.y += Maths::random()*spawnZone.y;
    //Global::player->position.z += Maths::random()*spawnZone.z;

    std::string playerSafeZoneLine;
    getlineSafe(file, playerSafeZoneLine);
    std::vector<std::string> safeZones = split(playerSafeZoneLine, ' ');
    Global::safeZoneStart = Vector3f(toF(safeZones[0]), toF(safeZones[1]), toF(safeZones[2]));
    Global::safeZoneEnd   = Vector3f(toF(safeZones[3]), toF(safeZones[4]), toF(safeZones[5]));

    std::string skyColorLine;
    getlineSafe(file, skyColorLine);
    //std::vector<std::string> skyColors = split(skyColorLine, ' ');
    //Global::skyColor = Vector3f(toF(skyColors[0]), toF(skyColors[1]), toF(skyColors[2]));

    std::string sunDirectionLine;
    getlineSafe(file, sunDirectionLine);
    //std::vector<std::string> sunDirection = split(sunDirectionLine, ' ');
    //Global::gameLightSun->direction = Vector3f(toF(sunDirection[0]), toF(sunDirection[1]), toF(sunDirection[2]));
    //Global::gameLightSun->direction.normalize();

    std::string camOrientationLine;
    getlineSafe(file, camOrientationLine);
    //std::vector<std::string> camOrientation = split(camOrientationLine, ' ');
    //Global::player->lookDir = Vector3f(toF(camOrientation[0]), toF(camOrientation[1]), toF(camOrientation[2]));
    //Global::player->lookDir.normalize();

    std::string roundTimeLine;
    getlineSafe(file, roundTimeLine);
    std::vector<std::string> roundTime = split(roundTimeLine, ' ');
    Global::timeUntilRoundEnds = toF(roundTime[0]);

    //printf("setting timeUntilRoundEnds to %f in levelloader\n", Global::timeUntilRoundEnds);

    //Now read through all the objects defined in the file

    std::string line;

    Global::gameEntitiesSharedMutex.lock();
    while (!file.eof())
    {
        getlineSafe(file, line);

        std::vector<std::string> tokens = split(line, ' ');

        if (tokens.size() > 0)
        {
            processLine(tokens);
        }
    }
    Global::gameEntitiesSharedMutex.unlock();

    file.close();

    // Spawn rock platforms
    if (Global::levelId == LVL_MAP4)
    {
        std::vector<RockPlatform*> rocksForTimers;
        for (int x = 0; x < 6; x++)
        {
            for (int z = 0; z < 6; z++)
            {
                int idx = z + 10*x;

                std::string id = std::to_string(idx);
                id = "RP" + id;

                RockPlatform* rock = new RockPlatform(id, Vector3f((x-3)*8.0f, 0, (z-3)*8.0f)); INCR_NEW("Entity");
                rocksForTimers.push_back(rock);
                Global::gameEntities.insert(rock);
            }
        }

        // Set timer for rocks that will break
        int rocksToDie = (int)rocksForTimers.size() - 5;

        for (int i = 0; i < rocksToDie; i++)
        {
            int idx = (int)(rocksForTimers.size()*Maths::random());
            rocksForTimers[idx]->timeUntilBreaks = 3.0f + 40*Maths::random();
            rocksForTimers.erase(rocksForTimers.begin() + idx);
        }
    }

    if (waitForSomeTime)
    {
        int waitTargetMillis = 1; //how long loading screen should show at least (in milliseconds)

        std::chrono::high_resolution_clock::time_point timeEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> time_span = timeEnd - timeStart;
        double durationMillis = time_span.count();

        int waitForMs = waitTargetMillis - (int)durationMillis;
        if (waitForMs > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(waitForMs));
        }
    }

    //glfwSetTime(0);
    //extern double timeOld;
    //timeOld = 0.0;
}

void LevelLoader::processLine(std::vector<std::string>& dat)
{
    if (dat[0][0] == '#')
    {
        return;
    }

    int id = std::stoi(dat[0]);

    switch (id)
    {
        case ENTITY_BALL:
        {
            Ball* ball = new Ball(dat[1], Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4])), Vector3f(0,0,0)); INCR_NEW("Entity");
            Global::gameEntities.insert(ball);
            break;
        }

        //case ENTITY_COLLISION_BLOCK:
        //{
        //    CollisionBlock* block = new CollisionBlock(dat[1],
        //        Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4])), 
        //        toI(dat[5]), toF(dat[6]), toF(dat[7]), toF(dat[8]), 
        //        (bool)toI(dat[9]), 
        //        toF(dat[10]));
        //    INCR_NEW("Entity");
        //    Global::gameEntities.insert(block);
        //    break;
        //}

        //case ENTITY_RED_BARREL:
        //{
        //    RedBarrel* barrel = new RedBarrel(dat[1], Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4]))); INCR_NEW("Entity");
        //    Global::gameEntities.insert(barrel);
        //    break;
        //}

        //case ENTITY_LADDER:
        //{
        //    Ladder* ladder = new Ladder(dat[1],
        //        Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4])),
        //        Vector3f(toF(dat[5]), toF(dat[6]), toF(dat[7]))); INCR_NEW("Entity");
        //    Global::gameEntities.insert(ladder);
        //    break;
        //}

        case ENTITY_HEALTH_CUBE:
        {
            HealthCube* health = new HealthCube(dat[1], Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4]))); INCR_NEW("Entity");
            Global::gameEntities.insert(health);
            break;
        }

        case ENTITY_GLASS:
        {
            Vector3f centerPos(Vector3f(toF(dat[3]), toF(dat[4]), toF(dat[5])));

            Vector3f topPos = centerPos; topPos.z += 2.4f;
            Vector3f botPos = centerPos; botPos.z -= 2.4f;

            Glass* glassTop = new Glass(dat[1], topPos); INCR_NEW("Entity");
            Glass* glassBot = new Glass(dat[2], botPos); INCR_NEW("Entity");

            float ran = Maths::random();
            if (ran > 0.5f)
            {
                glassTop->isReal = false;
            }
            else
            {
                glassBot->isReal = false;
            }

            Global::gameEntities.insert(glassTop);
            Global::gameEntities.insert(glassBot);
            break;
        }

        //case ENTITY_BOOM_BOX:
        //{
        //    BoomBox* box = new BoomBox(dat[1], Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4])), toF(dat[5])); INCR_NEW("Entity");
        //    Global::gameEntities.insert(box);
        //    break;
        //}

        case ENTITY_ROCK_PLATFORM:
        {
            RockPlatform* rock = new RockPlatform(dat[1], Vector3f(toF(dat[2]), toF(dat[3]), toF(dat[4]))); INCR_NEW("Entity");
            Global::gameEntities.insert(rock);
            break;
        }

        default:
        {
            return;
        }
    }
}

float LevelLoader::toF(std::string input)
{
    return std::stof(input);
}

int LevelLoader::toI(std::string input)
{
    return std::stoi(input);
}
