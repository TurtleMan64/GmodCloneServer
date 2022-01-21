#ifndef BAT_H
#define BAT_H

#include <list>
#include "entity.hpp"
#include "../toolbox/vector.hpp"

class Bat : public Entity
{
public:
    Bat(std::string name, Vector3f pos);

    bool isCollected = false;

    void step();

    int getEntityType();
};
#endif
