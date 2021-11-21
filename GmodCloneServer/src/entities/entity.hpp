#ifndef ENTITIES_H
#define ENTITIES_H

class CollisionModel;
class Triangle3D;

#include <list>
#include <vector>
#include "../toolbox/vector.hpp"
#include <string>

#define ENTITY_BLANK 0
#define ENTITY_NPC 1
#define ENTITY_BALL 2
#define ENTITY_ONLINE_PLAYER 3
#define ENTITY_COLLISION_BLOCK 4
#define ENTITY_RED_BARREL 5

class Entity
{
public:
    std::string name = "Default"; //Name will be Human1, Ball1, Human2, etc.
    Vector3f position;
    Vector3f vel;
    float rotX    = 0.0f;
    float rotY    = 0.0f;
    float rotZ    = 0.0f;
    float rotRoll = 0.0f;
    float scale   = 1.0f;

public:
    Entity();
    Entity(Vector3f* initialPosition, float rotX, float rotY, float rotZ, float scale);
    virtual ~Entity();

    virtual void step();

    void increasePosition(float dx, float dy, float dz);

    void increaseRotation(float dx, float dy, float dz);

    Vector3f* getPosition();
    void setPosition(Vector3f* newPosition);
    void setPosition(float newX, float newY, float newZ);

    const float getRotX();
    void setRotX(float newRotX);

    const float getRotY();
    void setRotY(float newRotY);

    const float getRotZ();
    void setRotZ(float newRotZ);

    const float getRotSpin();
    void setRotSpin(float newRotSpin);

    const float getScale();
    void setScale(float newScale);

    const float getX();

    const float getY();

    const float getZ();

    void setX(float newX);

    void setY(float newY);

    void setZ(float newZ);

    virtual int getEntityType();

    virtual std::vector<Triangle3D*>* getCollisionTriangles();

    virtual void getHit(Vector3f* hitPos, Vector3f* hitDir, int weapon);

    //puts data into buf. returns the num bytes written
    virtual int serialize(char* buf);

    virtual void loadFromSerializedBytes(char* buf);
};
#endif
