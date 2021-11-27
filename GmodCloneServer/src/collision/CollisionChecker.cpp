#include <vector>
#include <algorithm>

#include "collisionchecker.hpp"
#include "triangle3d.hpp"
#include "../toolbox/maths.hpp"
#include "../entities/entity.hpp"
#include "../main.hpp"

std::vector<Triangle3D*> CollisionChecker::triangles;

float CollisionChecker::chunkedTrianglesMinX = 0;
float CollisionChecker::chunkedTrianglesMinZ = 1;
float CollisionChecker::chunkedTrianglesChunkSize = 1;
int   CollisionChecker::chunkedTrianglesWidth = 1;
int   CollisionChecker::chunkedTrianglesHeight = 1;
std::vector<std::vector<Triangle3D*>*> CollisionChecker::mapOfTriangles;

CollisionResult CollisionChecker::checkCollision(Vector3f* point, float sphereRadius)
{
    return CollisionChecker::checkCollision(point->x, point->y, point->z, sphereRadius);
}

CollisionResult CollisionChecker::checkCollision(float x, float y, float z, float sphereRadius)
{
    CollisionResult result;

    Triangle3D* closestCollisionTriangle = nullptr;
    Entity* closestEntity = nullptr;
    float distanceToCollisionPosition = 1000000000.0f;
    Vector3f directionToCollisionPosition;

    std::vector<std::vector<Triangle3D*>*> chunks;

    addChunkToDataStruct(getTriangleChunk(x - sphereRadius, z - sphereRadius), &chunks);
    addChunkToDataStruct(getTriangleChunk(x - sphereRadius, z + sphereRadius), &chunks);
    addChunkToDataStruct(getTriangleChunk(x               , z               ), &chunks);
    addChunkToDataStruct(getTriangleChunk(x + sphereRadius, z - sphereRadius), &chunks);
    addChunkToDataStruct(getTriangleChunk(x + sphereRadius, z + sphereRadius), &chunks);

    float thisDist;
    Vector3f thisDir;
    Vector3f point(x, y, z);

    for (std::vector<Triangle3D*>* chunk : chunks)
    {
        for (Triangle3D* tri : *chunk)
        {
            if (Maths::sphereIntersectsTriangle(&point, sphereRadius, tri, &thisDist, &thisDir))
            {
                if (thisDist < distanceToCollisionPosition)
                {
                    closestCollisionTriangle = tri;
                    distanceToCollisionPosition = thisDist;
                    directionToCollisionPosition = thisDir;
                }
            }
        }
    }

    for (Entity* e : Global::gameEntities)
    {
        std::vector<Triangle3D*>* ts = e->getCollisionTriangles();
        if (ts != nullptr)
        {
            for (Triangle3D* tri : *ts)
            {
                if (Maths::sphereIntersectsTriangle(&point, sphereRadius, tri, &thisDist, &thisDir))
                {
                    if (thisDist < distanceToCollisionPosition)
                    {
                        closestCollisionTriangle = tri;
                        distanceToCollisionPosition = thisDist;
                        directionToCollisionPosition = thisDir;
                        closestEntity = e;
                    }
                }
            }
        }
    }

    if (closestCollisionTriangle != nullptr)
    {
        result.hit = true;
        result.directionToPosition = directionToCollisionPosition;
        result.distanceToPosition = distanceToCollisionPosition;
        result.tri = closestCollisionTriangle;
        result.entity = closestEntity;
        return result;
    }

    return result;
}

CollisionResult CollisionChecker::checkCollision(Vector3f* p1, Vector3f* p2)
{
    return CollisionChecker::checkCollision(p1->x, p1->y, p1->z, p2->x, p2->y, p2->z);
}

CollisionResult CollisionChecker::checkCollision(float x1, float y1, float z1, float x2, float y2, float z2)
{
    CollisionResult result;

    Triangle3D* closestCollisionTriangle = nullptr;
    Entity* closestEntity = nullptr;

    float distanceToCollisionPositionSquared = Vector3f(x2-x1, y2-y1, z2-z1).lengthSquared();

    std::vector<std::vector<Triangle3D*>*> chunks;

    addChunkToDataStruct(getTriangleChunk(x1, z1), &chunks);
    addChunkToDataStruct(getTriangleChunk(x1, z2), &chunks);
    addChunkToDataStruct(getTriangleChunk(x2, z1), &chunks);
    addChunkToDataStruct(getTriangleChunk(x2, z2), &chunks);

    Vector3f rayOrigin(x1, y1, z1);
    Vector3f rayDir(x2 - x1, y2 - y1, z2 - z1);
    Vector3f collidePosition;

    for (std::vector<Triangle3D*>* chunk : chunks)
    {
        for (Triangle3D* tri : *chunk)
        {
            if (Maths::raycastIntersectsTriangle(&rayOrigin, &rayDir, tri, &collidePosition))
            {
                float thisDistSquared = (rayOrigin - collidePosition).lengthSquared();

                if (thisDistSquared < distanceToCollisionPositionSquared)
                {
                    closestCollisionTriangle = tri;
                    distanceToCollisionPositionSquared = thisDistSquared;
                }
            }
        }
    }

    for (Entity* e : Global::gameEntities)
    {
        std::vector<Triangle3D*>* ts = e->getCollisionTriangles();
        if (ts != nullptr)
        {
            for (Triangle3D* tri : *ts)
            {
                if (Maths::raycastIntersectsTriangle(&rayOrigin, &rayDir, tri, &collidePosition))
                {
                    float thisDistSquared = (rayOrigin - collidePosition).lengthSquared();

                    if (thisDistSquared < distanceToCollisionPositionSquared)
                    {
                        closestCollisionTriangle = tri;
                        distanceToCollisionPositionSquared = thisDistSquared;
                        closestEntity = e;
                    }
                }
            }
        }
    }

    if (closestCollisionTriangle != nullptr)
    {
        result.hit = true;
        result.distanceToPosition = sqrtf(distanceToCollisionPositionSquared);
        result.tri = closestCollisionTriangle;
        result.entity = closestEntity;
        return result;
    }

    return result;
}

void CollisionChecker::deleteAllTriangles()
{
    for (int i = 0; i < CollisionChecker::triangles.size(); i++)
    {
        delete CollisionChecker::triangles[i]; INCR_DEL("Triangle3D");
        CollisionChecker::triangles[i] = nullptr;
    }

    CollisionChecker::triangles.clear();

    for (int i = 0; i < mapOfTriangles.size(); i++)
    {
        delete mapOfTriangles[i]; INCR_DEL("std::vector<Triangle3D*>");
    }
    mapOfTriangles.clear();
}

void CollisionChecker::addTriangle(Triangle3D* tri)
{
    CollisionChecker::triangles.push_back(tri);
}

void CollisionChecker::constructChunkDatastructure()
{
    for (int i = 0; i < mapOfTriangles.size(); i++)
    {
        delete mapOfTriangles[i]; INCR_DEL("std::vector<Triangle3D*>");
    }
    mapOfTriangles.clear();

    chunkedTrianglesMinX = triangles[0]->minX;
    chunkedTrianglesMinZ = triangles[0]->minZ;

    float maxX = triangles[0]->maxX;
    float maxZ = triangles[0]->maxZ;

    for (int i = 1; i < triangles.size(); i++)
    {
        Triangle3D* tri = triangles[i];

        if (tri->minX < chunkedTrianglesMinX)
        {
            chunkedTrianglesMinX = tri->minX;
        }

        if (tri->minZ < chunkedTrianglesMinZ)
        {
            chunkedTrianglesMinZ = tri->minZ;
        }

        if (tri->maxX > maxX)
        {
            maxX = tri->maxX;
        }

        if (tri->maxZ > maxZ)
        {
            maxZ = tri->maxZ;
        }
    }

    chunkedTrianglesChunkSize = 5.0f;

    chunkedTrianglesMinX--;
    chunkedTrianglesMinZ--;
    maxX++;
    maxZ++;

    chunkedTrianglesWidth  = (int)std::ceil((maxX - chunkedTrianglesMinX)/chunkedTrianglesChunkSize);
    chunkedTrianglesHeight = (int)std::ceil((maxZ - chunkedTrianglesMinZ)/chunkedTrianglesChunkSize);

    //printf("chunkedTrianglesWidth = %d\n", chunkedTrianglesWidth);
    //printf("chunkedTrianglesHeight = %d\n", chunkedTrianglesHeight);

    for (int x = 0; x < chunkedTrianglesWidth; x++)
    {
        for (int y = 0; y < chunkedTrianglesHeight; y++)
        {
            std::vector<Triangle3D*>* clump = new std::vector<Triangle3D*>(); INCR_NEW("std::vector<Trinalge3D*>");
            mapOfTriangles.push_back(clump);
        }
    }

    //printf("Created %d clumps\n", (int)mapOfTriangles.size());

    for (int i = 0; i < triangles.size(); i++)
    {
        Triangle3D* tri = triangles[i];
        int xIdxStart = (int)((tri->minX - chunkedTrianglesMinX)/chunkedTrianglesChunkSize);
        int zIdxStart = (int)((tri->minZ - chunkedTrianglesMinZ)/chunkedTrianglesChunkSize);
        int xIdxEnd   = (int)((tri->maxX - chunkedTrianglesMinX)/chunkedTrianglesChunkSize);
        int zIdxEnd   = (int)((tri->maxZ - chunkedTrianglesMinZ)/chunkedTrianglesChunkSize);

        for (int x = xIdxStart; x <= xIdxEnd; x++)
        {
            for (int z = zIdxStart; z <= zIdxEnd; z++)
            {
                mapOfTriangles[x + z*chunkedTrianglesWidth]->push_back(tri);
            }
        }
    }

    //printf("total triangles = %d\n", (int)triangles.size());

    //for (int i = 0; i < mapOfTriangles.size(); i++)
    {
        //printf("%d\n", (int)mapOfTriangles[i]->size());
    }
}

std::vector<Triangle3D*>* CollisionChecker::getTriangleChunk(float x, float z)
{
    int xIdx = (int)((x - chunkedTrianglesMinX)/chunkedTrianglesChunkSize);
    int zIdx = (int)((z - chunkedTrianglesMinZ)/chunkedTrianglesChunkSize);
    if (xIdx >= 0 && xIdx < chunkedTrianglesWidth &&
        zIdx >= 0 && zIdx < chunkedTrianglesHeight)
    {
        return mapOfTriangles[xIdx + zIdx*chunkedTrianglesWidth];
    }

    return nullptr;
}

void CollisionChecker::addChunkToDataStruct(std::vector<Triangle3D*>* chunkToAdd, std::vector<std::vector<Triangle3D*>*>* chunksToCheck)
{
    if (chunkToAdd == nullptr)
    {
        return;
    }

    if (std::find(chunksToCheck->begin(), chunksToCheck->end(), chunkToAdd) == chunksToCheck->end())
    {
        chunksToCheck->push_back(chunkToAdd);
    }
}
