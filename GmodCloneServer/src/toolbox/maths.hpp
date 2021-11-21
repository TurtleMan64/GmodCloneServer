#ifndef MATHS_H
#define MATHS_H

class Vector3f;
class Vector2f;
class Camera;
class Triangle3D;

#include <random>
#include <chrono>

class Maths
{
private:
    static std::mt19937* generatorUniform;
    static std::uniform_real_distribution<float>* distributionUniform;

    static std::default_random_engine* generatorNormal;
    static std::normal_distribution<float>* distributionNormal;

public:
    static const float PI;
    static const float E;

    static float toRadians(float deg);

    static float toDegrees(float rad);

    //Returns the difference between the two angles
    //ang1 and ang2 should be in degrees
    static float compareTwoAngles(float ang1, float ang2);

    static float interpolate(float num1, float num2, float percent);

    //assumes the size of int and float are 32 bits
    static int sign(float value);

    //clamps value between lower and higher
    static float clamp(float lower, float value, float higher);

    //clamps value between lower and higher
    static int clamp(int lower, int value, int higher);

    static unsigned int getSignBit(float value);

    //calculates the number of base 10 decimal digits in the number. 
    // examples: numDigits(38)    = 2
    //           numDigits(68932) = 5
    static int numDigits(int number);

    //
    static float approach(float initialValue, float terminalValue, float approachConstant, float timePassed);

    static float positionDeltaFromApproach(float initialVelValue, float terminalVelValue, float approachConstant, float timePassed);

    //result needs to be array of 3 doubles
    //theta is in radians
    static void rotatePoint(float result[],
        float a, float b, float c,
        float u, float v, float w,
        float x, float y, float z,
        float theta);

    //Point that axis goes through,
    //direction of axis,
    //point to rotate, 
    //angle of rotation, in radians
    static Vector3f rotatePoint(
        Vector3f* pointToRotate,
        Vector3f* axisOfRotation,
        float theta);

    //Given two vectors, linear rotate from the A to B by percent and return that new vector.
    //If the two vectors are too small or are too similar already, a copy of A is retured.
    static Vector3f interpolateVector(Vector3f* A, Vector3f* B, float percent);

    static Vector3f interpolateVectorDebug(Vector3f* A, Vector3f* B, float percent);

    //calculates the angle in radians between two vectors
    static float angleBetweenVectors(Vector3f* A, Vector3f* B);

    //given two points A and B, returns which one is closer to a point Test
    static Vector3f getCloserPoint(Vector3f* A, Vector3f* B, Vector3f* testPoint);

    //returns a new velocity vector that has undergone continous drag over a period of time. 
    //drag should be a negative number to actually drag the velocity vector
    static Vector3f applyDrag(Vector3f* velocity, float drag, float deltaTime);

    //returns a new velocity that has undergone continous drag over a period of time. 
    //drag should be a negative number to actually drag the velocity
    static float applyDrag(float velocity, float drag, float deltaTime);

    static Vector3f bounceVector(Vector3f* initialVelocity, Vector3f* surfaceNormal, float elasticity);

    /**
    * Calculate the x and z speeds relative to a plane based off
    * the previous position you are coming in from
    *
    * @param xspd the x speed that you are going at before collision
    * @param yspd the y speed that you are going at before collision
    * @param zspd the z speed that you are going at before collision
    * @param A the collision point on the triangle
    * @param normal the normal of the triangle
    */
    static Vector3f calculatePlaneSpeed(float xspd, float yspd, float zspd, Vector3f* normal);

    /** Returns the point on a sphere that has the given angles from the center
    * @param angH in radians
    * @param angV in radians
    * @param radius
    * @return
    */
    static Vector3f spherePositionFromAngles(float angH, float angV, float radius);

    //outputs in degrees
    static void sphereAnglesFromPosition(Vector3f* direction, float* outAngY, float* outAngZ);

    //Generates a uniformly distributed random position on a sphere of radius 1
    static Vector3f randomPointOnSphere();

    static Vector3f projectOntoPlane(Vector3f* A, Vector3f* normal);

    //projects a vector along a line
    static Vector3f projectAlongLine(Vector3f* A, Vector3f* line);

    //calculates an arbitrary vector that is perpendicular to the given vector vec
    static Vector3f calculatePerpendicular(Vector3f* vec);

    //returns uniform random float >= 0 and < 1
    static float random();

    //normal distribution mean = 0, std dev = 1
    static float nextGaussian();

    //returns uniform random float >= 0 and < 1
    static float nextUniform();

    //Given two vectors that are perpencicular to each other, calculate
    // a third vector that is perpendicular to both
    static Vector3f calcThirdAxis(Vector3f* A, Vector3f* B);

    //Calculate the A B C and D values for a plane from a normal and a point
    static Vector4f calcPlaneValues(Vector3f* point, Vector3f* normal);

    //Calculate the A B C and D values for a plane from 3 points
    static Vector4f calcPlaneValues(Vector3f* p1, Vector3f* p2, Vector3f* p3);

    //Given a basis {v1, v2, v3}, computes the coordinate vector
    // of a point a in the new basis
    static Vector3f coordinatesRelativeToBasis(Vector3f* v1, Vector3f* v2, Vector3f* v3, Vector3f* a);

    //tests if a point is inside a cylinder
    static bool pointIsInCylinder(Vector3f* point, Vector3f* cylinderEnd1, Vector3f* cylinderEnd2, float cylinderRadius);

    static Vector3f closestPointFromPointToLine(Vector3f* point, Vector3f* p1, Vector3f* p2);

    static bool sphereIntersectsTriangle(Vector3f* sphereCenter, float sphereRadius, Triangle3D* tri, float* outDistanceToTriangle, Vector3f* outDirectionToTriangle);

    static bool raycastIntersectsTriangle(Vector3f* rayOrigin, Vector3f* rayDir, Triangle3D* tri, Vector3f* outCollisionPosition);

    static bool lineSegmentIntersectsSphere(Vector3f* p1, Vector3f* p2, Vector3f* sphereCenter, float radius, Vector3f* outCollisionPosition);
};

#endif
