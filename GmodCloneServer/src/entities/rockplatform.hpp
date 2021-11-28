#ifndef ROCKPLATFORM_H
#define ROCKPLATFORM_H

#include <string>

#include "entity.hpp"
#include "../toolbox/vector.hpp"

class RockPlatform : public Entity
{
public:
    float timeUntilBreaks = 100000000.0f;

    RockPlatform(std::string name, Vector3f pos);
    ~RockPlatform();

    void step();

    int getEntityType();
};
#endif
