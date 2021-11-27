#include <list>
#include <set>

#include "../toolbox/vector.hpp"
#include "glass.hpp"
#include "../main.hpp"
#include "../toolbox/maths.hpp"
#include "../entities/entity.hpp"

extern float dt;

Glass::Glass(std::string name, Vector3f pos)
{
    this->name = name;

    position = pos;
}

Glass::~Glass()
{

}

void Glass::step()
{

}

int Glass::getEntityType()
{
    return ENTITY_GLASS;
}
