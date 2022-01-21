#include <list>
#include <vector>

#include "../toolbox/vector.hpp"
#include "bat.hpp"
#include "../main.hpp"
#include "../toolbox/maths.hpp"
#include "../loader/objloader.hpp"
#include "../entities/entity.hpp"

extern float dt;

Bat::Bat(std::string name, Vector3f pos)
{
    this->name = name;

    position = pos;
}

void Bat::step()
{

}


int Bat::getEntityType()
{
    return ENTITY_BAT;
}
