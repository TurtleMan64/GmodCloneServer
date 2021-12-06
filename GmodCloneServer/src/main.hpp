#include <unordered_set>
#include <string>
#include <shared_mutex>
#include <mutex>

#include "toolbox/vector.hpp"

#define DEV_MODE

#ifdef DEV_MODE
    #define INCR_NEW(NAME) Global::debugNew(NAME);
    #define INCR_DEL(NAME) Global::debugDel(NAME);
#else
    #define INCR_NEW(NAME) ;
    #define INCR_DEL(NAME) ;
#endif

class Entity;
class PlayerConnection;

#define LVL_HUB  0
#define LVL_MAP1 1
#define LVL_MAP2 2
#define LVL_MAP3 3
#define LVL_EQ   4
#define LVL_MAP4 5
#define LVL_TEST 6
#define LVL_MAP5 7

class Global
{
public:
    static std::string pathToEXE;

    //Use lock_shared when you are reading entity data (multiple threads can read at the same time)
    // Use lock when you are adding/deleting entities (prevents threads from reading while this is happening)
    static std::shared_mutex gameEntitiesSharedMutex;

    //Use this when you are modifying the state of an entity 
    static std::shared_mutex gameEntitiesMutex;

    static std::unordered_set<Entity*> gameEntities;

    static std::shared_mutex playerConnectionsSharedMutex;
    static std::unordered_set<PlayerConnection*> playerConnections;

    static int levelId;

    //static unsigned long long serverStartTime; //when the server started up
    //static unsigned long long getRawUtcSystemTime();

    static float timeUntilRoundStarts;
    static float timeUntilRoundEnds;

    static Vector3f safeZoneStart;
    static Vector3f safeZoneEnd;

    static void debugNew(const char* name);

    static void debugDel(const char* name);

    static void pickNextLevel();
};
