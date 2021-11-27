#ifndef GLASS_H
#define GLASS_H

#include <string>
#include "entity.hpp"
#include "../toolbox/vector.hpp"

class Glass : public Entity
{
public:
    bool isReal = true;
    bool hasBroken = false;

    Glass(std::string name, Vector3f pos);
    ~Glass();

    void step();

    int getEntityType();
};
#endif
