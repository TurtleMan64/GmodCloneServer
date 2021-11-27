#include <list>

#include "../toolbox/vector.hpp"
#include "healthcube.hpp"
#include "../main.hpp"
#include "../toolbox/maths.hpp"
#include "../loader/objloader.hpp"
#include "../entities/entity.hpp"

extern float dt;

HealthCube::HealthCube(std::string name, Vector3f pos)
{
    this->name = name;

    position = pos;

    isCollected = false;
}

void HealthCube::step()
{

}

int HealthCube::getEntityType()
{
    return ENTITY_HEALTH_CUBE;
}
