#include "entity.hpp"
#include "../toolbox/vector.hpp"
#include "../toolbox/maths.hpp"
#include "../main.hpp"
#include "../collision/collisionmodel.hpp"

#include <list>
#include <iostream>
#include <string>

Entity::Entity()
{
    this->position.x = 0;
    this->position.y = 0;
    this->position.z = 0;
    this->rotX = 0;
    this->rotY = 0;
    this->rotZ = 0; 
    this->rotRoll = 0;
    this->scale = 1;
}

Entity::Entity(Vector3f* position, float rotX, float rotY, float rotZ, float scale)
{
    this->position.x = position->x;
    this->position.y = position->y;
    this->position.z = position->z;
    this->rotX = rotX;
    this->rotY = rotY;
    this->rotZ = rotZ;
    this->rotRoll = 0;
    this->scale = scale;
}

Entity::~Entity()
{

}

void Entity::step()
{

}

int Entity::getEntityType()
{
    return 0;
}

std::vector<Triangle3D*>* Entity::getCollisionTriangles()
{
    return nullptr;
}

void Entity::getHit(Vector3f* /*hitPos*/, Vector3f* /*hitDir*/, int /*weapon*/)
{

}

int Entity::serialize(char* /*buf*/)
{
    return 0;
}

void Entity::loadFromSerializedBytes(char* /*buf*/)
{

}

void Entity::increasePosition(float dx, float dy, float dz)
{
    position.x += dx;
    position.y += dy;
    position.z += dz;
}

void Entity::increaseRotation(float dx, float dy, float dz)
{
    rotX += dx;
    rotY += dy;
    rotZ += dz;
}

Vector3f* Entity::getPosition()
{
    return &position;
}
void Entity::setPosition(Vector3f* newPos)
{
    this->position.x = newPos->x;
    this->position.y = newPos->y;
    this->position.z = newPos->z;
}
void Entity::setPosition(float newX, float newY, float newZ)
{
    this->position.x = newX;
    this->position.y = newY;
    this->position.z = newZ;
}

const float Entity::getRotX()
{
    return rotX;
}
void Entity::setRotX(float newRotX)
{
    rotX = newRotX;
}

const float Entity::getRotY()
{
    return rotY;
}
void Entity::setRotY(float newRotY)
{
    rotY = newRotY;
}

const float Entity::getRotZ()
{
    return rotZ;
}
void Entity::setRotZ(float newRotZ)
{
    rotZ = newRotZ;
}

const float Entity::getRotSpin()
{
    return rotRoll;
}
void Entity::setRotSpin(float newRotSpin)
{
    rotRoll = newRotSpin;
}

const float Entity::getScale()
{
    return scale;
}
void Entity::setScale(float newScale)
{
    scale = newScale;
}

const float Entity::getX()
{
    return position.x;
}

const float Entity::getY()
{
    return position.y;
}

const float Entity::getZ()
{
    return position.z;
}

void Entity::setX(float newX)
{
    position.x = newX;
}

void Entity::setY(float newY)
{
    position.y = newY;
}

void Entity::setZ(float newZ)
{
    position.z = newZ;
}
