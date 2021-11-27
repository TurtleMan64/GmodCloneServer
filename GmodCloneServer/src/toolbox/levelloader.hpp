#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <string>
#include <list>
#include <vector>

class LevelLoader
{
private:
    static float toF(std::string input);

    static int toI(std::string input);

    static void processLine(std::vector<std::string>& tokens);

public:
    static void loadLevel(std::string mapName);
};
#endif
