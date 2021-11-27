#ifndef HEALTHCUBE_H
#define HEALTHCUBE_H

#include <list>
#include "entity.hpp"
#include "../toolbox/vector.hpp"

class HealthCube : public Entity
{
public:
    bool isCollected = false;
    HealthCube(std::string name, Vector3f pos);

    void step();

    int getEntityType();
};
#endif
