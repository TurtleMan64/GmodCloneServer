#include "../toolbox/vector.hpp"
#include "rockplatform.hpp"
#include "../main.hpp"
#include "../entities/entity.hpp"

extern float dt;

RockPlatform::RockPlatform(std::string name, Vector3f pos)
{
    this->name = name;

    position = pos;
}

RockPlatform::~RockPlatform()
{

}

void RockPlatform::step()
{
  
}

int RockPlatform::getEntityType()
{
    return ENTITY_ROCK_PLATFORM;
}
