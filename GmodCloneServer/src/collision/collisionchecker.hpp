#ifndef COLLISIONCHECKER_H
#define COLLISIONCHECKER_H

class Triangle3D;
class Entity;

#include <vector>
#include "../toolbox/vector.hpp"

class CollisionResult
{
public:
    bool hit = false;
    Vector3f directionToPosition;
    float distanceToPosition = 0.0f;
    Triangle3D* tri = nullptr;
    Entity* entity = nullptr;
};

class CollisionChecker
{
private:
    static std::vector<Triangle3D*> triangles;

    static float chunkedTrianglesMinX;
    static float chunkedTrianglesMinZ;
    static float chunkedTrianglesChunkSize;
    static int chunkedTrianglesWidth;
    static int chunkedTrianglesHeight;
    static std::vector<std::vector<Triangle3D*>*> mapOfTriangles;

    static std::vector<Triangle3D*>* getTriangleChunk(float x, float z);

    static void addChunkToDataStruct(std::vector<Triangle3D*>* chunkToAdd, std::vector<std::vector<Triangle3D*>*>* chunksToCheck);

public:
    static CollisionResult checkCollision(Vector3f* p1, float sphereRadius);

    static CollisionResult checkCollision(float x, float y, float z, float sphereRadius);

    // Only use for short rays
    static CollisionResult checkCollision(Vector3f* p1, Vector3f* p2);

    // Only use for short rays
    static CollisionResult checkCollision(float x1, float y1, float z1, float x2, float y2, float z2);

    static void deleteAllTriangles();

    static void addTriangle(Triangle3D* cm);

    static void constructChunkDatastructure();
};

#endif
