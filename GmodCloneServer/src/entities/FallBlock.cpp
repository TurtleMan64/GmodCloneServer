#include <string>

#include "../toolbox/vector.hpp"
#include "fallblock.hpp"
#include "../main.hpp"
#include "../entities/entity.hpp"

FallBlock::FallBlock(std::string name, Vector3f pos, float phaseTimer)
{
    this->name = name;

    this->phaseTimer = phaseTimer;

    position = pos;
}

void FallBlock::step()
{
   
}

int FallBlock::getEntityType()
{
    return ENTITY_FALL_BLOCK;
}
