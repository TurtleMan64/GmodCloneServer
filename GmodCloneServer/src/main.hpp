#include <unordered_set>
#include <string>

#define INCR_NEW(NAME) ;
#define INCR_DEL(NAME) ;

class Entity;

class Global
{
public:
    static std::string pathToEXE;
    static std::unordered_set<Entity*> gameEntities;

    static unsigned long long serverStartTime; //when the server started up
    static unsigned long long getRawUtcSystemTime();
};
