#include <cmath>
#include <random>
#include <chrono>

#include "vector.hpp"
#include "../collision/triangle3d.hpp"
#include "maths.hpp"

std::mt19937* Maths::generatorUniform = new std::mt19937(0);
std::uniform_real_distribution<float>* Maths::distributionUniform = new std::uniform_real_distribution<float>(0.0f, 1.0f);

std::default_random_engine* Maths::generatorNormal = new std::default_random_engine(0);
std::normal_distribution<float>* Maths::distributionNormal = new std::normal_distribution<float>(0.0f, 1.0f);

const float Maths::PI = 3.14159265358979323846f;
const float Maths::E  = 2.71828182845904523536f;

float Maths::toRadians(float degrees)
{
    return (degrees*0.01745329251f);
}

float Maths::toDegrees(float radians)
{
    return (radians*57.2957795131f);
}

//Given two vectors that are perpencicular to each other, calculate
// a third vector that is perpendicular to both
Vector3f Maths::calcThirdAxis(Vector3f* A, Vector3f* B)
{
    //float third[3];
    //rotatePoint(third, 0, 0, 0, A->x, A->y, A->z, B->x, B->y, B->z, -(M_PI/2));

    //return Vector3f(third[0], third[1], third[2]);

    return B->cross(A);
}

//Given a basis {v1, v2, v3}, computes the coordinate vector
// of a point a in the new basis
Vector3f Maths::coordinatesRelativeToBasis(Vector3f* v1, Vector3f* v2, Vector3f* v3, Vector3f* a)
{
    // Set up the augmented matrix
    float A[3][4] = {{v1->x, v2->x, v3->x, a->x},
                     {v1->y, v2->y, v3->y, a->y},
                     {v1->z, v2->z, v3->z, a->z}};

    const int nrows = 3; // number of rows
    const int ncols = 4; // number of columns

    int lead = 0; 

    // Row reduce
    while (lead < nrows)
    {
        float d, m;

        for (int r = 0; r < nrows; r++)
        {
            d = A[lead][lead];
            m = A[r][lead] / A[lead][lead];

            for (int c = 0; c < ncols; c++)
            {
                if (r == lead)
                {
                    A[r][c] /= d;
                }
                else
                {
                    A[r][c] -= A[lead][c] * m;
                }
            }
        }

        lead++;
    }

    return Vector3f(A[0][3], A[1][3], A[2][3]);
}

//Returns the difference between the two angles
//ang1 and ang2 should be in degrees
float Maths::compareTwoAngles(float origAng1, float origAng2)
{
    float ang1 = fmodf(fmodf(origAng1, 360) + 360, 360);
    float ang2 = fmodf(fmodf(origAng2, 360) + 360, 360);

    float d = fmodf(fabsf(ang1 - ang2), 360);
    float r = d > 180 ? 360 - d : d;

    //calculate sign
    int sign = (ang1 - ang2 >= 0 && ang1 - ang2 <= 180) || (ang1 - ang2 <= -180 && ang1 - ang2 >= -360) ? 1 : -1;
    r *= sign;

    return r;
}

float Maths::interpolate(float a, float b, float percent)
{
    return (a*(1.0f - percent)) + (b*percent);
}

int Maths::sign(float value)
{
    //int v = *(int*)&value; //get bits of value casted as an int

    //return 1 - 2*((v>>31) & 0x01);

    if (value > 0)
    {
        return 1;
    }
    else if (value < 0)
    {
        return -1;
    }
    return 0;
}

//clamps value between lower and higher
float Maths::clamp(float lower, float value, float higher)
{
    return fmaxf(lower, fminf(value, higher));
}

//clamps value between lower and higher
int Maths::clamp(int lower, int value, int higher)
{
    return std::max(lower, std::min(value, higher));
}

unsigned int Maths::getSignBit(float value)
{
    unsigned int v = *(unsigned int*)&value; //get bits of value casted as an int

    return v & 0b10000000000000000000000000000000;
}

int Maths::numDigits(int number)
{
    int numChars = (int)ceil(log10f((float)(number+1)));
    if (number == 0)
    {
        numChars = 1;
    }
    return numChars;
}

float Maths::approach(float initialValue, float terminalValue, float approachConstant, float timePassed)
{
    //return ((initialValue-terminalValue)*powf(Maths::E, -approachConstant*timePassed) + terminalValue);

    float answer1 = ((initialValue-terminalValue)*powf(Maths::E, -approachConstant*timePassed) + terminalValue);

    //float A = approachConstant*2;
    //float T = terminalValue;
    //float v0 = initialValue;
    //float dt = timePassed;
    //float e = Maths::E;
    //
    //float posDelta = ((powf(e, -A*dt)*(T - v0)) + A*T*dt + v0 - T)/A;
    //float answer2 = posDelta/timePassed;

    //float A = approachConstant;
    //float T = terminalValue;
    //float v0 = initialValue;
    //float dt = timePassed;
    //float e = Maths::E;
    //
    //float pos1 = (powf(e, -A*0)*(A*0*T*powf(e, A*0) - v0 + T))/A;
    //float pos2 = (powf(e, -A*dt)*(A*dt*T*powf(e, A*dt) - v0 + T))/A;
    //
    //float answer3 = (pos2-pos1)/timePassed;

    //printf("diff = %f\n", (answer2-answer1));
    return answer1;
}

float Maths::positionDeltaFromApproach(float initialVelValue, float terminalVelValue, float approachConstant, float timePassed)
{
    float A = approachConstant*2; //not sure why i needed to add this
    float T = terminalVelValue;
    float v0 = initialVelValue;
    float dt = timePassed;
    float e = Maths::E;

    return ((powf(e, -A*dt)*(T - v0)) + A*T*dt + v0 - T)/A;
}

Vector3f Maths::applyDrag(Vector3f* velocity, float drag, float deltaTime)
{
    float length = velocity->length();
    if (length < 0.0001f)
    {
        return Vector3f(velocity);
    }
    float newLength = length*powf(Maths::E, drag*deltaTime);
    return velocity->scaleCopy(newLength/length);
}

float Maths::applyDrag(float velocity, float drag, float deltaTime)
{
    float length = fabsf(velocity);
    if (length < 0.0001f)
    {
        return velocity;
    }
    float newLength = length*powf(Maths::E, drag*deltaTime);
    return velocity*(newLength/length);
}

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
Vector3f Maths::calculatePlaneSpeed(float xspd, float yspd, float zspd, Vector3f* normal)
{
    Vector3f A(xspd, yspd, zspd);
    Vector3f Blue = projectOntoPlane(&A, normal);

    //Rotate normal along y axis 90 degrees
    float CDir = atan2f(-normal->z, -normal->x);
    CDir += (Maths::PI / 2);
    float Cx = cosf(CDir);
    float Cz = sinf(CDir);

    float CDist = sqrtf(normal->x*normal->x + normal->z*normal->z);
    float CPitch = ((Maths::PI / 2) + atan2f(-normal->y, CDist));

    float result[3] = { 0, 0, 0 }; //storage for the answer
    rotatePoint(result, 0, 0, 0, Cx, 0, Cz, Blue.x, Blue.y, Blue.z, -CPitch);

    return Vector3f(result[0], result[1], result[2]);
}

//Equation from https://sites.google.com/site/glennmurray/Home/rotation-matrices-and-formulas
//Point that axis goes through,
//direction of axis,
//point to rotate, 
//angle of rotation
void Maths::rotatePoint(float result[],
    float a, float b, float c,
    float u, float v, float w,
    float x, float y, float z,
    float theta)
{
    if (sqrtf(u*u + v*v + w*w) < 0.000000001f)
    {
        //std::fprintf(stdout, "Warning: trying to rotate by a very small axis [%f %f %f]\n", u, v, w);
        result[0] = x;
        result[1] = y;
        result[2] = z;
        return;
    }

    float u2 = u*u;
    float v2 = v*v;
    float w2 = w*w;
    float l2 = u2 + v2 + w2;
    float l = sqrtf(l2);

    float cosT = cosf(theta);
    float oneMinusCosT = 1 - cosT;
    float sinT = sinf(theta);

    result[0] = ((a*(v2 + w2) - u*(b*v + c*w - u*x - v*y - w*z)) * oneMinusCosT
        + l2*x*cosT
        + l*(-c*v + b*w - w*y + v*z)*sinT) / l2;

    result[1] = ((b*(u2 + w2) - v*(a*u + c*w - u*x - v*y - w*z)) * oneMinusCosT
        + l2*y*cosT
        + l*(c*u - a*w + w*x - u*z)*sinT) / l2;

    result[2] = ((c*(u2 + v2) - w*(a*u + b*v - u*x - v*y - w*z)) * oneMinusCosT
        + l2*z*cosT
        + l*(-b*u + a*v - v*x + u*y)*sinT) / l2;
}

//Point to rotate
//direction of axis,
//angle of rotation, in radians
Vector3f Maths::rotatePoint(
    Vector3f* pointToRotate,
    Vector3f* axisOfRotation,
    float theta)
{
    float result[3];
    Maths::rotatePoint(result, 0, 0, 0, 
        axisOfRotation->x, 
        axisOfRotation->y, 
        axisOfRotation->z, 
        pointToRotate->x, 
        pointToRotate->y, 
        pointToRotate->z, 
        theta);

    return Vector3f(result[0], result[1], result[2]);
}

Vector3f Maths::interpolateVector(Vector3f* A, Vector3f* B, float percent)
{
    Vector3f perpen = A->cross(B);
    float dotProduct = A->dot(B);
    float mag = A->length()*B->length();

    if (mag < 0.0000001f)
    {
        //std::fprintf(stdout, "Warning: Trying to interpolate between small vectors\n");
        return Vector3f(A);
    }

    float similatiry = dotProduct/mag;

    if (similatiry > 0.9999999f) //Vectors are extremely similar already, just return A
    {
        return Vector3f(A);
    }

    float angle = acos(similatiry);
    percent = fminf(1.0f, fmaxf(0.0f, percent));
    return Maths::rotatePoint(A, &perpen, angle*percent);
}

Vector3f Maths::interpolateVectorDebug(Vector3f* A, Vector3f* B, float percent)
{
    Vector3f perpen = A->cross(B);
    float dotProduct = A->dot(B);
    float mag = A->length()*B->length();

    if (mag < 0.0000001f)
    {
        std::fprintf(stdout, "1\n");
        return Vector3f(A);
    }

    if (dotProduct/mag > 0.9999999f)
    {
        std::fprintf(stdout, "2\n");
        return Vector3f(A);
    }

    float angle = acos(dotProduct/mag);
    percent = fminf(1.0f, fmaxf(0.0f, percent));
    std::fprintf(stdout, "percent=%f\n", percent);
    return Maths::rotatePoint(A, &perpen, angle*percent);
}

float Maths::angleBetweenVectors(Vector3f* A, Vector3f* B)
{
    float mag = A->length()*B->length();

    if (mag < 0.0000001f)
    {
        return 0;
    }

    Vector3f A2(A);
    Vector3f B2(B);
    A2.normalize();
    B2.normalize();

    float dotProduct = A2.dot(&B2);

    if (dotProduct >= 1.0f) //Vectors are extremely similar, return 0
    {
        return 0;
    }
    else if (dotProduct <= -1.0f) //Vectors are opposite direction, return pi
    {
        return Maths::PI;
    }

    return acosf(dotProduct);
}

Vector3f Maths::getCloserPoint(Vector3f* A, Vector3f* B, Vector3f* testPoint)
{
    Vector3f deltaA(A);
    deltaA = deltaA - testPoint;
    Vector3f deltaB(B);
    deltaB = deltaB - testPoint;
    float distA = deltaA.lengthSquared();
    float distB = deltaB.lengthSquared();
    if (distA < distB)
    {
        return Vector3f(A);
    }
    return Vector3f(B);
}

//https://stackoverflow.com/questions/11132681/what-is-a-formula-to-get-a-vector-perpendicular-to-another-vector
Vector3f Maths::calculatePerpendicular(Vector3f* vec)
{
    bool b0 = (vec->x <  vec->y) && (vec->x <  vec->z);
    bool b1 = (vec->y <= vec->x) && (vec->y <  vec->z);
    bool b2 = (vec->z <= vec->x) && (vec->z <= vec->y);
    Vector3f differentVec((float)(b0), (float)(b1), (float)(b2));
    differentVec.setLength(1);
    Vector3f perpen = vec->cross(&differentVec);
    perpen.setLength(1);
    return perpen;
}

Vector3f Maths::projectAlongLine(Vector3f* A, Vector3f* line)
{
    Vector3f master(A);
    //std::fprintf(stdout, "master = %f %f %f\n", master.x, master.y, master.z);
    Vector3f perpen1 = Maths::calculatePerpendicular(line);
    perpen1.normalize();
    //std::fprintf(stdout, "perpen1 = %f %f %f\n", perpen1.x, perpen1.y, perpen1.z);
    Vector3f perpen2 = perpen1.cross(line);
    perpen2.normalize();
    //std::fprintf(stdout, "perpen2 = %f %f %f\n", perpen2.x, perpen2.y, perpen2.z);
    master = Maths::projectOntoPlane(&master, &perpen1);
    //std::fprintf(stdout, "master = %f %f %f\n", master.x, master.y, master.z);
    master = Maths::projectOntoPlane(&master, &perpen2);
    //std::fprintf(stdout, "master = %f %f %f\n", master.x, master.y, master.z);
    return master;
}

/**
* @param initialVelocity
* @param surfaceNormal
* @param elasticity Scale of the resulting vector relative to the original velocity
*/
Vector3f Maths::bounceVector(Vector3f* initialVelocity, Vector3f* surfaceNormal, float elasticity)
{
    Vector3f twoNtimesVdotN(surfaceNormal);
    twoNtimesVdotN.scale(-2 * initialVelocity->dot(surfaceNormal));

    Vector3f Vnew = (twoNtimesVdotN + initialVelocity);
    Vnew.scale(elasticity);

    return Vnew;
}


//Projects vector A to be perpendicular to vector normal
Vector3f Maths::projectOntoPlane(Vector3f* A, Vector3f* normal)
{
    Vector3f B(0, 0, 0);
    Vector3f C(A);
    Vector3f N(normal->x, normal->y, normal->z);
    N.normalize();

    N.scale(C.dot(&N));
    B = C - N;

    return B;
}

/** Returns the point on a sphere that has the given angles from the center
* @param angH in radians
* @param angV in radians
* @param radius
* @return
*/
Vector3f Maths::spherePositionFromAngles(float angH, float angV, float radius)
{
    float y   = (radius*sinf(angV));
    float hpt = (radius*cosf(angV));

    float x = (hpt*cosf(angH));
    float z = (hpt*sinf(angH));

    return Vector3f(x, y, z);
}

void Maths::sphereAnglesFromPosition(Vector3f* dir, float* outAngY, float* outAngZ)
{
    *outAngY = Maths::toDegrees(atan2f(-dir->z, dir->x));
    float distH = sqrtf(dir->z*dir->z + dir->x*dir->x);
    *outAngZ = Maths::toDegrees(atan2f(dir->y, distH));
}

Vector3f Maths::randomPointOnSphere()
{
    float z   = Maths::nextUniform()*2 - 1;
    float lng = Maths::nextUniform()*2*Maths::PI;

    float radius = sqrtf(1-(z)*(z));

    float x = radius*cosf(lng);
    float y = radius*sinf(lng);

    return Vector3f(x, y, z);


    //Makes a 3D Bernoulli Lemniscate ???

    //float theta = 2 * M_PI * Maths::nextUniform();
    //float phi = acosf(1 - 2 * Maths::nextUniform());
    //float x = sinf(phi) * cosf(theta);
    //float y = sinf(phi) * sinf(theta);
    //float z = cosf(phi);

    //return spherePositionFromAngles(x, y, z);
}

float Maths::random()
{
    return (rand() % RAND_MAX) / ((float)(RAND_MAX));
}

float Maths::nextGaussian()
{
    return (*Maths::distributionNormal)(*Maths::generatorNormal);
}

float Maths::nextUniform()
{
    return (*Maths::distributionUniform)(*Maths::generatorUniform);
}

Vector4f Maths::calcPlaneValues(Vector3f* p1, Vector3f* p2, Vector3f* p3)
{
    Vector3f vec1(p1->x - p3->x, p1->y - p3->y, p1->z - p3->z);
    Vector3f vec2(p2->x - p3->x, p2->y - p3->y, p2->z - p3->z);

    Vector3f cross = vec1.cross(&vec2);

    float newD = cross.x*p3->x + cross.y*p3->y + cross.z*p3->z;

    return Vector4f(cross.x, cross.y, cross.z, -newD);
}

Vector4f Maths::calcPlaneValues(Vector3f* point, Vector3f* normal)
{
    Vector3f perp1 = Maths::calculatePerpendicular(normal);
    Vector3f perp2 = perp1.cross(normal);

    Vector3f p1(point);
    Vector3f p2(point);
    Vector3f p3(point);
    p2 = p2 + perp1;
    p3 = p3 + perp2;

    return Maths::calcPlaneValues(&p1, &p2, &p3);
}

bool Maths::pointIsInCylinder(Vector3f* point, Vector3f* c1, Vector3f* c2, float cRadius)
{
    Vector3f xAxis(1, 0, 0);
    Vector3f cDiff(c2);
    cDiff = cDiff - c1;

    float cLength = cDiff.length();
    float angDiff = Maths::angleBetweenVectors(&cDiff, &xAxis);

    Vector3f p(point);
    p = p - c1; //move c1 to origin

    if (fabsf(angDiff) < 0.0001f)
    {
        //skip rotate if the cylinder already is aligned with x axis
    }
    else if (fabsf(angDiff) > Maths::PI - 0.0001f)
    {
        //rotate 180 degrees
        Vector3f yAxis(0, 1, 0);
        p = Maths::rotatePoint(&p, &yAxis, Maths::toRadians(180));
    }
    else
    {
        Vector3f perpen = xAxis.cross(&cDiff);
        p = Maths::rotatePoint(&p, &perpen, -angDiff); //rotate so cylinder faces xAxis
    }

    return (p.x >= 0 &&
            p.x < cLength &&
            p.y*p.y + p.z*p.z < cRadius*cRadius);
}

//https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
Vector3f Maths::closestPointFromPointToLine(Vector3f* p, Vector3f* l1, Vector3f* l2)
{
    Vector3f line = *l2 - l1;
    float lineLengthSquared = line.lengthSquared();
    //printf("line length = %f\n", lineLengthSquared);
    if (lineLengthSquared <= 0.0000000001f)
    {
        //printf("bad\n");
        return *l1;
    }

    float t = ((p->x - l1->x) * (l2->x - l1->x) + (p->y - l1->y) * (l2->y - l1->y) + (p->z - l1->z) * (l2->z - l1->z)) / lineLengthSquared;
    //printf("t = %f\n", t);
    t = Maths::clamp(0.0f, t, 1.0f);

    return Vector3f(l1->x + t * (l2->x - l1->x), l1->y + t * (l2->y - l1->y), l1->z + t * (l2->z - l1->z));
}

//https://stackoverflow.com/questions/34043955/detect-collision-between-sphere-and-triangle-in-three-js
bool Maths::sphereIntersectsTriangle(Vector3f* sphereCenter, float sphereRadius, Triangle3D* tri, float* outDistanceToTriangle, Vector3f* outDirectionToTriangle)
{
    Vector3f p1(tri->p1X, tri->p1Y, tri->p1Z);
    Vector3f p2(tri->p2X, tri->p2Y, tri->p2Z);
    Vector3f p3(tri->p3X, tri->p3Y, tri->p3Z);

    Vector3f closestPoint1 = Maths::closestPointFromPointToLine(sphereCenter, &p1, &p2);
    Vector3f closestPoint2 = Maths::closestPointFromPointToLine(sphereCenter, &p1, &p3);
    Vector3f closestPoint3 = Maths::closestPointFromPointToLine(sphereCenter, &p2, &p3);

    float sphereRadiusSquared = sphereRadius*sphereRadius;
    bool intersects = false;
    float minDistToTriangleSquared = 10000000000.0f;

    float distToCenterSquared = (closestPoint1 - sphereCenter).lengthSquared();
    if (distToCenterSquared <= sphereRadiusSquared)
    {
        intersects = true;
        minDistToTriangleSquared = distToCenterSquared;
        Vector3f dir = closestPoint1 - sphereCenter;
        dir.normalize();
        outDirectionToTriangle->set(&dir);
    }
    
    distToCenterSquared = (closestPoint2 - sphereCenter).lengthSquared();
    if (distToCenterSquared <= sphereRadiusSquared)
    {
        intersects = true;
        if (distToCenterSquared < minDistToTriangleSquared)
        {
            minDistToTriangleSquared = distToCenterSquared;
            Vector3f dir = closestPoint2 - sphereCenter;
            dir.normalize();
            outDirectionToTriangle->set(&dir);
        }
    }

    distToCenterSquared = (closestPoint3 - sphereCenter).lengthSquared();
    if (distToCenterSquared <= sphereRadiusSquared)
    {
        intersects = true;
        if (distToCenterSquared < minDistToTriangleSquared)
        {
            minDistToTriangleSquared = distToCenterSquared;
            Vector3f dir = closestPoint3 - sphereCenter;
            dir.normalize();
            outDirectionToTriangle->set(&dir);
        }
    }

    Vector3f startCast = (*sphereCenter) + tri->normal.scaleCopy(sphereRadius*5);
    Vector3f castDirection = tri->normal.scaleCopy(-1);
    Vector3f colPos;
    if (Maths::raycastIntersectsTriangle(&startCast, &castDirection, tri, &colPos))
    {
        Vector3f diff = colPos - sphereCenter;
        float lengthSquared = diff.lengthSquared();
        if (lengthSquared < sphereRadius*sphereRadius)
        {
            intersects = true;
            if (lengthSquared < minDistToTriangleSquared)
            {
                minDistToTriangleSquared = lengthSquared;
                castDirection.normalize();
                outDirectionToTriangle->set(&castDirection);
            }
        }
    }

    *outDistanceToTriangle = sqrtf(minDistToTriangleSquared);
    return intersects;
}

bool Maths::raycastIntersectsTriangle(Vector3f* rayOrigin, Vector3f* rayDir, Triangle3D* tri, Vector3f* outCollisionPosition)
{
    Vector3f R = *rayOrigin;
    Vector3f Rdir = *rayDir;
    Rdir.normalize();

    Vector3f A(tri->p1X, tri->p1Y, tri->p1Z);
    Vector3f B(tri->p2X, tri->p2Y, tri->p2Z);
    Vector3f C(tri->p3X, tri->p3Y, tri->p3Z);

    Vector3f E1 = B - A;
    Vector3f E2 = C - A;
    Vector3f N = E1.cross(&E2);
    float det = -(Rdir.dot(&N));
    if (det == 0.0f)
    {
        //printf("uh oh\n");
        return false;
    }
    float invdet = 1.0f/det;
    Vector3f AO  = R - A;
    Vector3f DAO = AO.cross(&Rdir);
    float u =  (E2.dot(&DAO)) * invdet;
    float v = -(E1.dot(&DAO)) * invdet;
    float t =  (AO.dot(&N))   * invdet;

    bool intersects = (det >= 1e-6f && t >= 0.0f && u >= 0.0f && v >= 0.0f && (u+v) <= 1.0f);

    if (intersects)
    {
        Vector3f intersectPoint = R + Rdir.scaleCopy(t);
        outCollisionPosition->set(&intersectPoint);
    }

    return intersects;
}

//http://paulbourke.net/geometry/circlesphere/index.html#linesphere
//http://paulbourke.net/geometry/circlesphere/raysphere.c
bool Maths::lineSegmentIntersectsSphere(Vector3f* p1, Vector3f* p2, Vector3f* sc, float r, Vector3f* outCollisionPosition)
{
    float a,b,c;
    float bb4ac;
    Vector3f dp;

    if ((*p1 - *sc).lengthSquared() < r*r)
    {
        // If start point is already in the sphere, return collide point as start point
        outCollisionPosition->set(p1);
        return true;
    }

    dp.x = p2->x - p1->x;
    dp.y = p2->y - p1->y;
    dp.z = p2->z - p1->z;
    a = dp.x * dp.x + dp.y * dp.y + dp.z * dp.z;
    b = 2 * (dp.x * (p1->x - sc->x) + dp.y * (p1->y - sc->y) + dp.z * (p1->z - sc->z));
    c = sc->x * sc->x + sc->y * sc->y + sc->z * sc->z;
    c += p1->x * p1->x + p1->y * p1->y + p1->z * p1->z;
    c -= 2 * (sc->x * p1->x + sc->y * p1->y + sc->z * p1->z);
    c -= r * r;
    bb4ac = b * b - 4 * a * c;

    if (fabsf(a) < 0.000001f || bb4ac < 0)
    {
        return false;
    }

    float mu1 = (-b + sqrt(bb4ac)) / (2 * a);
    float mu2 = (-b - sqrt(bb4ac)) / (2 * a);

    Vector3f intp1 = *p1 + (*p2 - *p1).scaleCopy(mu1);
    Vector3f intp2 = *p1 + (*p2 - *p1).scaleCopy(mu2);

    if ((intp1 - *p1).lengthSquared() < (intp2 - *p1).lengthSquared())
    {
        outCollisionPosition->set(&intp1);
    }
    else
    {
        outCollisionPosition->set(&intp2);
    }

    if ((*p1 - *outCollisionPosition).lengthSquared() < (*p1 - *p2).lengthSquared())
    {
        return true;
    }

    return false;
}
