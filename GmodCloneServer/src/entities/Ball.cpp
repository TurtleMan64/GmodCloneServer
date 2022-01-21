#include <list>
#include <set>

#include "../toolbox/vector.hpp"
#include "ball.hpp"
#include "../collision/collisionchecker.hpp"
#include "../collision/triangle3d.hpp"
#include "../main.hpp"
#include "../toolbox/maths.hpp"

extern float dt;

Ball::Ball(std::string name, Vector3f pos, Vector3f vel)
{
    this->name = name;

    scale = 0.225f; //Size 5 soccer ball is ~22.5 cm
    position = pos;
    this->vel = vel;
}

void Ball::step()
{
    Vector3f yAxis(0, 1, 0);

    vel = vel - yAxis.scaleCopy(gravityForce*dt);

    position = position + vel.scaleCopy(dt);

    CollisionResult result = CollisionChecker::checkCollision(&position, scale);
    if (result.hit)
    {
        // First, resolve the collision
        float distanceToMoveAway = scale - result.distanceToPosition;
        Vector3f directionToMove = result.directionToPosition.scaleCopy(-1);
        position = position + directionToMove.scaleCopy(distanceToMoveAway);

        // Then, bounce
        Vector3f velBounce = Maths::bounceVector(&vel, &directionToMove, 1.0f);

        Vector3f bounceUp = Maths::projectAlongLine(&velBounce, &directionToMove);

        if (bounceUp.lengthSquared() > 1*1)
        {
            //AudioPlayer::play(12, &position);
        }

        bounceUp.scale(1 - bounceAmount);
        vel = velBounce - bounceUp;
    }

    vel = Maths::applyDrag(&vel, -DRAG_AIR, dt); //Slow vel down due to air drag

    Vector3f rotP = vel;
    rotP.y = 0;
    rotZ -= 150*rotP.length()*dt;
    rotY = Maths::toDegrees(atan2f(-vel.z, vel.x));
}

int Ball::getEntityType()
{
    return ENTITY_BALL;
}

void Ball::getHit(Vector3f* /*hitPos*/, Vector3f* hitDir, int weapon)
{
    vel = *hitDir;

    if (weapon == 0)
    {
        vel.setLength(10.0f);
    }
    else
    {
        vel.setLength(30.0f);
    }
}

int Ball::serialize(char* buf)
{
    memcpy(&buf[ 0], &position.x, 4);
    memcpy(&buf[ 4], &position.y, 4);
    memcpy(&buf[ 8], &position.z, 4);
    memcpy(&buf[12], &vel     .x, 4);
    memcpy(&buf[16], &vel     .y, 4);
    memcpy(&buf[20], &vel     .z, 4);

    return 24;
}

void Ball::loadFromSerializedBytes(char* buf)
{
    memcpy(&position.x, &buf[ 0], 4);
    memcpy(&position.y, &buf[ 4], 4);
    memcpy(&position.z, &buf[ 8], 4);
    memcpy(&vel     .x, &buf[12], 4);
    memcpy(&vel     .y, &buf[16], 4);
    memcpy(&vel     .z, &buf[20], 4);
}
