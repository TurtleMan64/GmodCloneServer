#include <cmath>
#include <list>

#include "triangle3d.hpp"
#include "../toolbox/vector.hpp"
#include "collisionmodel.hpp"
#include "../toolbox/maths.hpp"


CollisionModel::CollisionModel()
{

}

void CollisionModel::generateMinMaxValues()
{
    if (triangles.size() == 0)
    {
        return;
    }

    minX = triangles.front()->minX;
    maxX = triangles.front()->maxX;
    minY = triangles.front()->minY;
    maxY = triangles.front()->maxY;
    minZ = triangles.front()->minZ;
    maxZ = triangles.front()->maxZ;

    for (Triangle3D* tri : triangles)
    {
        minX = fmin(minX, tri->minX);
        maxX = fmax(maxX, tri->maxX);
        minY = fmin(minY, tri->minY);
        maxY = fmax(maxY, tri->maxY);
        minZ = fmin(minZ, tri->minZ);
        maxZ = fmax(maxZ, tri->maxZ);
    }
}

void CollisionModel::offsetModel(Vector3f* offset)
{
    float x = offset->x;
    float y = offset->y;
    float z = offset->z;

    for (Triangle3D* tri : triangles)
    {
        tri->p1X += x;
        tri->p2X += x;
        tri->p3X += x;
        tri->p1Y += y;
        tri->p2Y += y;
        tri->p3Y += y;
        tri->p1Z += z;
        tri->p2Z += z;
        tri->p3Z += z;

        tri->generateValues();
    }

    generateMinMaxValues();
}

void CollisionModel::rotateModelY(float yRot, Vector3f* center)
{
    float angleRad = Maths::toRadians(yRot);
    float cosAng = cosf(angleRad);
    float sinAng = sinf(angleRad);
    for (Triangle3D* tri : triangles)
    {
        float xDiff = tri->p1X - center->x;
        float zDiff = tri->p1Z - center->z;
        tri->p1X = center->x + ((xDiff)*cosAng - (zDiff)*sinAng);
        tri->p1Z = center->z - (-(zDiff)*cosAng - (xDiff)*sinAng);

        xDiff = tri->p2X - center->x;
        zDiff = tri->p2Z - center->z;
        tri->p2X = center->x + ((xDiff)*cosAng - (zDiff)*sinAng);
        tri->p2Z = center->z - (-(zDiff)*cosAng - (xDiff)*sinAng);

        xDiff = tri->p3X - center->x;
        zDiff = tri->p3Z - center->z;
        tri->p3X = center->x + ((xDiff)*cosAng - (zDiff)*sinAng);
        tri->p3Z = center->z - (-(zDiff)*cosAng - (xDiff)*sinAng);

        tri->generateValues();
    }

    generateMinMaxValues();
}

//rotation order: z, y
void CollisionModel::transformModel(CollisionModel* targetModel, Vector3f* translate, float yRot, float zRot)
{
    if (targetModel->triangles.size() != triangles.size())
    {
        std::fprintf(stdout, "Warning: Trying to transform a collision model based on another collision model that doesn't have the same amount of triangles.\n");
        return;
    }

    float angleRadY = Maths::toRadians(yRot);
    float angleRadZ = Maths::toRadians(zRot);

    Vector3f yAxis(0, 1, 0);
    Vector3f zAxis(0, 0, 1);

    int i = 0;
    for (Triangle3D* tri : triangles)
    {
        Vector3f newPoint1(tri->p1X, tri->p1Y, tri->p1Z);
        newPoint1 = Maths::rotatePoint(&newPoint1, &zAxis, angleRadZ);
        newPoint1 = Maths::rotatePoint(&newPoint1, &yAxis, angleRadY);
        newPoint1 = newPoint1 + translate;

        Vector3f newPoint2(tri->p2X, tri->p2Y, tri->p2Z);
        newPoint2 = Maths::rotatePoint(&newPoint2, &zAxis, angleRadZ);
        newPoint2 = Maths::rotatePoint(&newPoint2, &yAxis, angleRadY);
        newPoint2 = newPoint2 + translate;

        Vector3f newPoint3(tri->p3X, tri->p3Y, tri->p3Z);
        newPoint3 = Maths::rotatePoint(&newPoint3, &zAxis, angleRadZ);
        newPoint3 = Maths::rotatePoint(&newPoint3, &yAxis, angleRadY);
        newPoint3 = newPoint3 + translate;

        //new, keep already allocated triangles and change their values
        Triangle3D* newTri = targetModel->triangles[i];
        newTri->p1X      = newPoint1.x;
        newTri->p1Y      = newPoint1.y;
        newTri->p1Z      = newPoint1.z;
        newTri->p2X      = newPoint2.x;
        newTri->p2Y      = newPoint2.y;
        newTri->p2Z      = newPoint2.z;
        newTri->p3X      = newPoint3.x;
        newTri->p3Y      = newPoint3.y;
        newTri->p3Z      = newPoint3.z;
        newTri->type     = tri->type;
        newTri->sound    = tri->sound;
        newTri->particle = tri->particle;
        newTri->generateValues();
        i++;
    }

    targetModel->generateMinMaxValues();
}

//order = x, y, z
void CollisionModel::transformModel(CollisionModel* targetModel, Vector3f* translate, float xRot, float yRot, float zRot)
{
    if (targetModel->triangles.size() != triangles.size())
    {
        std::fprintf(stdout, "Warning: Trying to transform a collision model based on another collision model that doesn't have the same amount of triangles.\n");
        return;
    }

    float angleRadX = Maths::toRadians(xRot);
    float angleRadY = Maths::toRadians(yRot);
    float angleRadZ = Maths::toRadians(zRot);

    Vector3f xAxis(1, 0, 0);
    Vector3f yAxis(0, 1, 0);
    Vector3f zAxis(0, 0, 1);

    int i = 0;
    for (Triangle3D* tri : triangles)
    {
        Vector3f newPoint1(tri->p1X, tri->p1Y, tri->p1Z);
        newPoint1 = Maths::rotatePoint(&newPoint1, &xAxis, angleRadX);
        newPoint1 = Maths::rotatePoint(&newPoint1, &yAxis, angleRadY);
        newPoint1 = Maths::rotatePoint(&newPoint1, &zAxis, angleRadZ);
        newPoint1 = newPoint1 + translate;

        Vector3f newPoint2(tri->p2X, tri->p2Y, tri->p2Z);
        newPoint2 = Maths::rotatePoint(&newPoint2, &xAxis, angleRadX);
        newPoint2 = Maths::rotatePoint(&newPoint2, &yAxis, angleRadY);
        newPoint2 = Maths::rotatePoint(&newPoint2, &zAxis, angleRadZ);
        newPoint2 = newPoint2 + translate;

        Vector3f newPoint3(tri->p3X, tri->p3Y, tri->p3Z);
        newPoint3 = Maths::rotatePoint(&newPoint3, &xAxis, angleRadX);
        newPoint3 = Maths::rotatePoint(&newPoint3, &yAxis, angleRadY);
        newPoint3 = Maths::rotatePoint(&newPoint3, &zAxis, angleRadZ);
        newPoint3 = newPoint3 + translate;

        //keep already allocated triangles and change their values
        Triangle3D* newTri = targetModel->triangles[i];
        newTri->p1X      = newPoint1.x;
        newTri->p1Y      = newPoint1.y;
        newTri->p1Z      = newPoint1.z;
        newTri->p2X      = newPoint2.x;
        newTri->p2Y      = newPoint2.y;
        newTri->p2Z      = newPoint2.z;
        newTri->p3X      = newPoint3.x;
        newTri->p3Y      = newPoint3.y;
        newTri->p3Z      = newPoint3.z;
        newTri->type     = tri->type;
        newTri->sound    = tri->sound;
        newTri->particle = tri->particle;
        newTri->generateValues();
        i++;
    }

    targetModel->generateMinMaxValues();
}

//makes a collision model be the transformed version of this collision model
void CollisionModel::transformModel(CollisionModel* targetModel, Vector3f* translate, float yRot)
{
    if (targetModel->triangles.size() != triangles.size())
    {
        std::fprintf(stdout, "Warning: Trying to transform a collision model based on another collision model that doesn't have the same amount of triangles.\n");
        return;
    }

    float angleRadY = Maths::toRadians(yRot);

    Vector3f yAxis(0, 1, 0);

    int i = 0;
    for (Triangle3D* tri : triangles)
    {
        Vector3f newPoint1(tri->p1X, tri->p1Y, tri->p1Z);
        newPoint1 = Maths::rotatePoint(&newPoint1, &yAxis, angleRadY);
        newPoint1 = newPoint1 + translate;

        Vector3f newPoint2(tri->p2X, tri->p2Y, tri->p2Z);
        newPoint2 = Maths::rotatePoint(&newPoint2, &yAxis, angleRadY);
        newPoint2 = newPoint2 + translate;

        Vector3f newPoint3(tri->p3X, tri->p3Y, tri->p3Z);
        newPoint3 = Maths::rotatePoint(&newPoint3, &yAxis, angleRadY);
        newPoint3 = newPoint3 + translate;

        Triangle3D* newTri = targetModel->triangles[i];
        newTri->p1X      = newPoint1.x;
        newTri->p1Y      = newPoint1.y;
        newTri->p1Z      = newPoint1.z;
        newTri->p2X      = newPoint2.x;
        newTri->p2Y      = newPoint2.y;
        newTri->p2Z      = newPoint2.z;
        newTri->p3X      = newPoint3.x;
        newTri->p3Y      = newPoint3.y;
        newTri->p3Z      = newPoint3.z;
        newTri->type     = tri->type;
        newTri->sound    = tri->sound;
        newTri->particle = tri->particle;
        newTri->generateValues();
        i++;
    }

    targetModel->generateMinMaxValues();
}

void CollisionModel::transformModelWithScale(CollisionModel* targetModel, Vector3f* translate, float yRot, float scale)
{
    if (targetModel->triangles.size() != triangles.size())
    {
        std::fprintf(stdout, "Warning: Trying to transform a collision model based on another collision model that doesn't have the same amount of triangles.\n");
        return;
    }

    float angleRadY = Maths::toRadians(yRot);

    Vector3f yAxis(0, 1, 0);

    int i = 0;
    for (Triangle3D* tri : triangles)
    {
        Vector3f newPoint1(tri->p1X, tri->p1Y, tri->p1Z);
        newPoint1.scale(scale);
        newPoint1 = Maths::rotatePoint(&newPoint1, &yAxis, angleRadY);
        newPoint1 = newPoint1 + translate;

        Vector3f newPoint2(tri->p2X, tri->p2Y, tri->p2Z);
        newPoint2.scale(scale);
        newPoint2 = Maths::rotatePoint(&newPoint2, &yAxis, angleRadY);
        newPoint2 = newPoint2 + translate;

        Vector3f newPoint3(tri->p3X, tri->p3Y, tri->p3Z);
        newPoint3.scale(scale);
        newPoint3 = Maths::rotatePoint(&newPoint3, &yAxis, angleRadY);
        newPoint3 = newPoint3 + translate;

        Triangle3D* newTri = targetModel->triangles[i];
        newTri->p1X      = newPoint1.x;
        newTri->p1Y      = newPoint1.y;
        newTri->p1Z      = newPoint1.z;
        newTri->p2X      = newPoint2.x;
        newTri->p2Y      = newPoint2.y;
        newTri->p2Z      = newPoint2.z;
        newTri->p3X      = newPoint3.x;
        newTri->p3Y      = newPoint3.y;
        newTri->p3Z      = newPoint3.z;
        newTri->type     = tri->type;
        newTri->sound    = tri->sound;
        newTri->particle = tri->particle;
        newTri->generateValues();
        i++;
    }

    targetModel->generateMinMaxValues();
}

//makes a collision model be the transformed version of this collision model
void CollisionModel::transformModel(CollisionModel* targetModel, Vector3f* translate)
{
    if (targetModel->triangles.size() != triangles.size())
    {
        std::fprintf(stdout, "Warning: Trying to transform a collision model based on another collision model that doesn't have the same amount of triangles.\n");
        return;
    }

    float offX = translate->x;
    float offY = translate->y;
    float offZ = translate->z;

    //targetModel->deleteMe();
    int i = 0;
    for (Triangle3D* tri : triangles)
    {
        Vector3f newP1(offX + tri->p1X, offY + tri->p1Y, offZ + tri->p1Z);
        Vector3f newP2(offX + tri->p2X, offY + tri->p2Y, offZ + tri->p2Z);
        Vector3f newP3(offX + tri->p3X, offY + tri->p3Y, offZ + tri->p3Z);

        //Triangle3D* newTri = new Triangle3D(&newP1, &newP2, &newP3, tri->type, tri->sound, tri->particle); INCR_NEW("Triangle3D");

        //targetModel->triangles.push_back(newTri);

        Triangle3D* newTri = targetModel->triangles[i];
        newTri->p1X      = newP1.x;
        newTri->p1Y      = newP1.y;
        newTri->p1Z      = newP1.z;
        newTri->p2X      = newP2.x;
        newTri->p2Y      = newP2.y;
        newTri->p2Z      = newP2.z;
        newTri->p3X      = newP3.x;
        newTri->p3Y      = newP3.y;
        newTri->p3Z      = newP3.z;
        newTri->type     = tri->type;
        newTri->sound    = tri->sound;
        newTri->particle = tri->particle;
        newTri->generateValues();
        i++;
    }

    targetModel->generateMinMaxValues();
}

void CollisionModel::deleteMe()
{
    //Delete triangles 
    for (Triangle3D* tri : triangles)
    {
        delete tri;
    }

    triangles.clear();
}

CollisionModel* CollisionModel::duplicateMe()
{
    CollisionModel* copy = new CollisionModel;

    copy->maxX            = this->maxX;
    copy->minX            = this->minX;
    copy->maxY            = this->maxY;
    copy->minY            = this->minY;
    copy->maxZ            = this->maxZ;
    copy->minZ            = this->minZ;

    for (Triangle3D* tri : triangles)
    {
        Vector3f v1(tri->p1X, tri->p1Y, tri->p1Z);
        Vector3f v2(tri->p2X, tri->p2Y, tri->p2Z);
        Vector3f v3(tri->p3X, tri->p3Y, tri->p3Z);
        Triangle3D* newTri = new Triangle3D(&v1, &v2, &v3, tri->type, tri->sound, tri->particle);
        newTri->generateValues();
        copy->triangles.push_back(newTri);
    }

    return copy;
}
