#ifndef FALLBLOCK_H
#define FALLBLOCK_H

#include <string>
#include "entity.hpp"
#include "../toolbox/vector.hpp"

class FallBlock : public Entity
{
public:
    float phaseTimer = 100000000.0f;

    FallBlock(std::string name, Vector3f pos, float phaseTimer);

    void step();

    int getEntityType();
};
#endif
