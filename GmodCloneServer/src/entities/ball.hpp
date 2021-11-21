#ifndef BALL_H
#define BALL_H

class TexturedModel;

#include <list>
#include "entity.hpp"
#include "../toolbox/vector.hpp"

class Ball : public Entity
{
private:
    const float gravityForce = 0.588f*60;

    const float DRAG_AIR = 0.4f;

    const float bounceAmount = 0.85f; //85% of relative vertical speed retain after bounce

public:
    Ball(std::string name, Vector3f pos, Vector3f vel);

    void step();

    int getEntityType();

    void getHit(Vector3f* hitPos, Vector3f* hitDir, int weapon);

    //puts data into buf. returns the num bytes written
    virtual int serialize(char* buf);

    virtual void loadFromSerializedBytes(char* buf);
};
#endif
