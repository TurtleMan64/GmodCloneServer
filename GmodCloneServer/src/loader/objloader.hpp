#ifndef OBJLOADER_H
#define OBJLOADER_H

class TexturedModel;
class CollisionModel;
class Vertex;
class QuadTreeNode;

#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdio.h>
#include "../textures/modeltexture.hpp"
#include "../toolbox/vector.hpp"

class ObjLoader
{
private:
    static void parseMtl(std::string filePath, std::string fileName, std::unordered_map<std::string, ModelTexture>* outMtlMap);

    static void deleteUnusedMtl(std::unordered_map<std::string, ModelTexture>* mtlMap, std::vector<ModelTexture>* usedMtls);

    static void processVertex(char** vertex,
        std::vector<Vertex*>* vertices,
        std::vector<int>* indices);

    static void processVertexBinary(int, int, int,
        std::vector<Vertex*>* vertices,
        std::vector<int>* indices);

    static void dealWithAlreadyProcessedVertex(Vertex*, 
        int, 
        int, 
        std::vector<int>*, 
        std::vector<Vertex*>*);

    static void removeUnusedVertices(std::vector<Vertex*>* vertices);

    static void convertDataToArrays(
        std::vector<Vertex*>* vertices, 
        std::vector<Vector2f>* textures,
        std::vector<Vector3f>* normals, 
        std::vector<float>* verticesArray, 
        std::vector<float>* texturesArray,
        std::vector<float>* normalsArray,
        std::vector<float>* colorsArray);

public:
    //The CollisionModel returned must be deleted later.
    static CollisionModel* loadCollisionModel(std::string filePath, std::string fileName);

    //The CollisionModel returned must be deleted later.
    static CollisionModel* loadBinaryCollisionModel(std::string filePath, std::string fileName);
};
#endif
