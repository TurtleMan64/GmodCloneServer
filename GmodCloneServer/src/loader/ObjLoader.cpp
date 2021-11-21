#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <list>

#include "../collision/collisionmodel.hpp"
#include "objLoader.hpp"
#include "../toolbox/vector.hpp"
#include "vertex.hpp"
#include "../main.hpp"
#include "../toolbox/split.hpp"
#include "../toolbox/getline.hpp"
#include "../collision/triangle3d.hpp"
#include "fakeTexture.hpp"
#include "../toolbox/maths.hpp"

void ObjLoader::parseMtl(std::string filePath, std::string fileName, std::unordered_map<std::string, ModelTexture>* outMtlMap)
{
    //map that we fill in, from the mtl file
    if (!outMtlMap->empty())
    {
        std::fprintf(stderr, "Warning: Material map is not empty.\n");
    }
    outMtlMap->clear();

    std::ifstream file(Global::pathToEXE+filePath+fileName);
    if (!file.is_open())
    {
        std::fprintf(stderr, "Error: Cannot load file '%s'\n", (Global::pathToEXE + filePath + fileName).c_str());
        file.close();
        return;
    }

    //clock_t t;
    //t = clock();
    //printf("Calculating MTL...\n");

    std::string line;

    //default values
    std::string currentMaterialName = "DefaultMtl";
    float currentShineDamperValue = 20.0f;
    float currentReflectivityValue = 0.0f;
    float currentTransparencyValue = 1.0f;
    float currentFakeLightingValue = 1.0f;
    float currentGlowAmountValue = 0.0f;
    float currentScrollXValue = 0.0f;
    float currentScrollYValue = 0.0f;
    int   currentNumImages = 1;
    float currentAnimSpeed = 0.0f;
    int   currentMixingType = 1;
    float currentFogScale = 1.0f;
    int   currentRenderOrder = 0;

    while (!file.eof())
    {
        getlineSafe(file, line);

        char lineBuf[256];
        memcpy(lineBuf, line.c_str(), line.size()+1);

        int splitLength = 0;
        char** lineSplit = split(lineBuf, ' ', &splitLength);

        if (splitLength > 0)
        {
            if (strcmp(lineSplit[0], "newmtl") == 0) //new material found, add its name to array
            {
                currentMaterialName = lineSplit[1];
                currentShineDamperValue = 0.0f;
                currentReflectivityValue = 0.0f;
                currentTransparencyValue = 1.0f;
                currentFakeLightingValue = 1.0f;
                currentGlowAmountValue = 0.0f;
                currentScrollXValue = 0.0f;
                currentScrollYValue = 0.0f;
                currentNumImages = 1;
                currentAnimSpeed = 0.0f;
                currentMixingType = 1;
                currentFogScale = 1.0f;
                currentRenderOrder = 0;
            }
            else if (strcmp(lineSplit[0], "\tmap_Kd") == 0 || strcmp(lineSplit[0], "map_Kd") == 0) //end of material found, generate it with all its attrributes
            {
                std::string imageFilenameString = filePath+lineSplit[1];
                char* fname = (char*)imageFilenameString.c_str();

                //std::vector<GLuint> textureIds;
                //textureIds.push_back(Loader::loadTexture(fname)); //generate new texture

                currentNumImages--;
                while (currentNumImages > 0)
                {
                    free(lineSplit);

                    getlineSafe(file, line);

                    memcpy(lineBuf, line.c_str(), line.size()+1);

                    lineSplit = split(lineBuf, ' ', &splitLength);

                    char* nextFilename = lineSplit[0];

                    if (lineSplit[0][0] == '\t')
                    {
                        nextFilename = &lineSplit[0][1];
                    }

                    imageFilenameString = filePath+nextFilename;
                    fname = (char*)imageFilenameString.c_str();
                    //textureIds.push_back(Loader::loadTexture(fname)); //load the new texture

                    currentNumImages--;
                }

                ModelTexture newTexture;

                newTexture.shineDamper = currentShineDamperValue;
                newTexture.reflectivity = currentReflectivityValue;
                newTexture.hasTransparency = true;
                newTexture.useFakeLighting = false;
                if (currentTransparencyValue > 0.0f)
                {
                    newTexture.hasTransparency = false;
                }
                if (currentFakeLightingValue < 1.0f)
                {
                    newTexture.useFakeLighting = true;
                }
                newTexture.glowAmount = currentGlowAmountValue;
                newTexture.scrollX = currentScrollXValue;
                newTexture.scrollY = currentScrollYValue;
                newTexture.animationSpeed = currentAnimSpeed;
                newTexture.mixingType = currentMixingType;
                newTexture.fogScale = currentFogScale;
                newTexture.renderOrder = (char)currentRenderOrder;

                (*outMtlMap)[currentMaterialName] = newTexture; //put a copy of newTexture into the list
            }
            else if (strcmp(lineSplit[0], "\tNs") == 0 || strcmp(lineSplit[0], "Ns") == 0)
            {
                currentShineDamperValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\tNi") == 0 || strcmp(lineSplit[0], "Ni") == 0)
            {
                currentReflectivityValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\tTr") == 0 || strcmp(lineSplit[0], "Tr") == 0)
            {
                currentTransparencyValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\td") == 0 || strcmp(lineSplit[0], "d") == 0)
            {
                currentFakeLightingValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\tglow") == 0 || strcmp(lineSplit[0], "glow") == 0)
            {
                currentGlowAmountValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\tscrollX") == 0 || strcmp(lineSplit[0], "scrollX") == 0)
            {
                currentScrollXValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\tscrollY") == 0 || strcmp(lineSplit[0], "scrollY") == 0)
            {
                currentScrollYValue = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\tanimSpeed") == 0 || strcmp(lineSplit[0], "animSpeed") == 0)
            {
                currentAnimSpeed = std::stof(lineSplit[1]);
                if (currentAnimSpeed < 0)
                {
                    std::fprintf(stderr, "Error: animSpeed was negative.\n");
                    currentAnimSpeed = 0;
                }
            }
            else if (strcmp(lineSplit[0], "\tnumImages") == 0 || strcmp(lineSplit[0], "numImages") == 0)
            {
                currentNumImages = std::stoi(lineSplit[1]);
                if (currentNumImages < 1)
                {
                    std::fprintf(stderr, "Error: numImages was negative.\n");
                    currentNumImages = 1;
                }
            }
            else if (strcmp(lineSplit[0], "\tmixLinear") == 0 || strcmp(lineSplit[0], "mixLinear") == 0)
            {
                currentMixingType = 2;
            }
            else if (strcmp(lineSplit[0], "\tmixSinusoidal") == 0 || strcmp(lineSplit[0], "mixSinusoidal") == 0)
            {
                currentMixingType = 3;
            }
            else if (strcmp(lineSplit[0], "\tfogScale") == 0 || strcmp(lineSplit[0], "fogScale") == 0)
            {
                currentFogScale = std::stof(lineSplit[1]);
            }
            else if (strcmp(lineSplit[0], "\trenderOrder") == 0 || strcmp(lineSplit[0], "renderOrder") == 0)
            {
                currentRenderOrder = Maths::clamp(0, std::stoi(lineSplit[1]), 4);
            }
        }

        free(lineSplit);
    }
    file.close();

    //t = clock() - t;
    //printf("MTL: It took me %d clicks (%f seconds).\n", t, ((float)t) / CLOCKS_PER_SEC);

}


void ObjLoader::processVertex(char** vertex,
    std::vector<Vertex*>* vertices,
    std::vector<int>* indices)
{
    int index = atoi(vertex[0]) - 1;
    int textureIndex = atoi(vertex[1]) - 1;
    int normalIndex = atoi(vertex[2]) - 1;

    Vertex* currentVertex = (*vertices)[index]; //check bounds on this?
    if (currentVertex->isSet() == 0)
    {
        currentVertex->setTextureIndex(textureIndex);
        currentVertex->setNormalIndex(normalIndex);
        indices->push_back(index);
    }
    else
    {
        dealWithAlreadyProcessedVertex(currentVertex, textureIndex, normalIndex, indices, vertices);
    }
}

void ObjLoader::processVertexBinary(int vIndex, int tIndex, int nIndex,
    std::vector<Vertex*>* vertices,
    std::vector<int>* indices)
{
    vIndex--;
    tIndex--;
    nIndex--;

    Vertex* currentVertex = (*vertices)[vIndex]; //check bounds on this?
    if (currentVertex->isSet() == 0)
    {
        currentVertex->setTextureIndex(tIndex);
        currentVertex->setNormalIndex(nIndex);
        indices->push_back(vIndex);
    }
    else
    {
        dealWithAlreadyProcessedVertex(currentVertex, tIndex, nIndex, indices, vertices);
    }
}

void ObjLoader::dealWithAlreadyProcessedVertex(
    Vertex* previousVertex,
    int newTextureIndex,
    int newNormalIndex,
    std::vector<int>* indices,
    std::vector<Vertex*>* vertices)
{
    if (previousVertex->hasSameTextureAndNormal(newTextureIndex, newNormalIndex))
    {
        indices->push_back(previousVertex->getIndex());
    }
    else
    {
        Vertex* anotherVertex = previousVertex->getDuplicateVertex();
        if (anotherVertex != nullptr)
        {
            dealWithAlreadyProcessedVertex(anotherVertex, newTextureIndex, newNormalIndex, indices, vertices);
        }
        else
        {
            Vertex* duplicateVertex = new Vertex((int)vertices->size(), previousVertex->getPosition(), &previousVertex->color); INCR_NEW("Vertex");
            //numAdditionalVertices++;
            duplicateVertex->setTextureIndex(newTextureIndex);
            duplicateVertex->setNormalIndex(newNormalIndex);

            previousVertex->setDuplicateVertex(duplicateVertex);
            vertices->push_back(duplicateVertex);
            indices->push_back(duplicateVertex->getIndex());
        }
    }
}


void ObjLoader::convertDataToArrays(
    std::vector<Vertex*>* vertices, 
    std::vector<Vector2f>* textures,
    std::vector<Vector3f>* normals, 
    std::vector<float>* verticesArray, 
    std::vector<float>* texturesArray,
    std::vector<float>* normalsArray,
    std::vector<float>* colorsArray)
{
    for (auto currentVertex : (*vertices))
    {
        Vector3f* position = currentVertex->getPosition();
        Vector2f* textureCoord = &(*textures)[currentVertex->getTextureIndex()];
        Vector3f* normalVector = &(*normals)[currentVertex->getNormalIndex()];
        verticesArray->push_back(position->x);
        verticesArray->push_back(position->y);
        verticesArray->push_back(position->z);
        texturesArray->push_back(textureCoord->x);
        texturesArray->push_back(1 - textureCoord->y);
        normalsArray->push_back(normalVector->x);
        normalsArray->push_back(normalVector->y);
        normalsArray->push_back(normalVector->z);
        colorsArray->push_back(currentVertex->color.x);
        colorsArray->push_back(currentVertex->color.y);
        colorsArray->push_back(currentVertex->color.z);
        colorsArray->push_back(currentVertex->color.w);
    }
}

void ObjLoader::removeUnusedVertices(std::vector<Vertex*>* vertices)
{
    for (auto vertex : (*vertices))
    {
        if (vertex->isSet() == 0)
        {
            vertex->setTextureIndex(0);
            vertex->setNormalIndex(0);
        }
    }
}

CollisionModel* ObjLoader::loadCollisionModel(std::string filePath, std::string fileName)
{
    CollisionModel* collisionModel = new CollisionModel; INCR_NEW("CollisionModel");

    std::list<FakeTexture> fakeTextures;

    char currType = 0;
    char currSound = 0;
    char currParticle = 0;

    std::ifstream file(Global::pathToEXE + "res/" + filePath + fileName + ".obj");
    if (!file.is_open())
    {
        std::fprintf(stdout, "Error: Cannot load file '%s'\n", (Global::pathToEXE + "res/" + filePath + fileName + ".obj").c_str());
        file.close();
        return collisionModel;
    }

    std::string line;

    std::vector<Vector3f> vertices;



    while (!file.eof())
    {
        getlineSafe(file, line);

        char lineBuf[256];
        memcpy(lineBuf, line.c_str(), line.size()+1);

        int splitLength = 0;
        char** lineSplit = split(lineBuf, ' ', &splitLength);

        if (splitLength > 0)
        {
            if (strcmp(lineSplit[0], "v") == 0)
            {
                Vector3f vertex;
                vertex.x = std::stof(lineSplit[1]);
                vertex.y = std::stof(lineSplit[2]);
                vertex.z = std::stof(lineSplit[3]);
                vertices.push_back(vertex);
            }
            else if (strcmp(lineSplit[0], "f") == 0)
            {
                int len = 0;
                char** vertex1 = split(lineSplit[1], '/', &len);
                char** vertex2 = split(lineSplit[2], '/', &len);
                char** vertex3 = split(lineSplit[3], '/', &len);

                Vector3f* vert1 = &vertices[std::stoi(vertex1[0]) - 1];
                Vector3f* vert2 = &vertices[std::stoi(vertex2[0]) - 1];
                Vector3f* vert3 = &vertices[std::stoi(vertex3[0]) - 1];

                Triangle3D* tri = new Triangle3D(vert1, vert2, vert3, currType, currSound, currParticle); INCR_NEW("Triangle3D");

                collisionModel->triangles.push_back(tri);

                free(vertex1);
                free(vertex2);
                free(vertex3);
            }
            else if (strcmp(lineSplit[0], "usemtl") == 0)
            {
                currType = 0;
                currSound = -1;
                currParticle = 0;

                for (FakeTexture dummy : fakeTextures)
                {
                    if (dummy.name == lineSplit[1])
                    {
                        currType = dummy.type;
                        currSound = dummy.sound;
                        currParticle = dummy.particle;
                    }
                }
            }
            else if (strcmp(lineSplit[0], "mtllib") == 0)
            {
                std::ifstream fileMTL(Global::pathToEXE + "res/" + filePath + lineSplit[1]);
                if (!fileMTL.is_open())
                {
                    std::fprintf(stdout, "Error: Cannot load file '%s'\n", (Global::pathToEXE + "res/" + filePath + lineSplit[1]).c_str());
                    fileMTL.close();
                    file.close();
                    return collisionModel;
                }

                std::string lineMTL;

                while (!fileMTL.eof())
                {
                    getlineSafe(fileMTL, lineMTL);

                    char lineBufMTL[256];
                    memcpy(lineBufMTL, lineMTL.c_str(), lineMTL.size()+1);

                    int splitLengthMTL = 0;
                    char** lineSplitMTL = split(lineBufMTL, ' ', &splitLengthMTL);

                    if (splitLengthMTL > 1)
                    {
                        if (strcmp(lineSplitMTL[0], "newmtl") == 0)
                        {
                            FakeTexture fktex;

                            fktex.name = lineSplitMTL[1];
                            fakeTextures.push_back(fktex);
                        }
                        else if (strcmp(lineSplitMTL[0], "type") == 0 ||
                                 strcmp(lineSplitMTL[0], "\ttype") == 0)
                        {
                            if (strcmp(lineSplitMTL[1], "bounce") == 0)
                            {
                                fakeTextures.back().type = 1;
                            }
                            else if (strcmp(lineSplitMTL[1], "death") == 0)
                            {
                                fakeTextures.back().type = 2;
                            }
                        }
                        else if (strcmp(lineSplitMTL[0], "sound") == 0 ||
                                 strcmp(lineSplitMTL[0], "\tsound") == 0)
                        {
                            if (strcmp(lineSplitMTL[1], "concrete") == 0)
                            {
                                fakeTextures.back().sound = 0;
                            }
                            else if (strcmp(lineSplitMTL[1], "dirt") == 0)
                            {
                                fakeTextures.back().sound = 1;
                            }
                            else if (strcmp(lineSplitMTL[1], "grass") == 0)
                            {
                                fakeTextures.back().sound = 2;
                            }
                            else if (strcmp(lineSplitMTL[1], "metal") == 0)
                            {
                                fakeTextures.back().sound = 3;
                            }
                            else if (strcmp(lineSplitMTL[1], "snow") == 0)
                            {
                                fakeTextures.back().sound = 4;
                            }
                            else if (strcmp(lineSplitMTL[1], "water") == 0)
                            {
                                fakeTextures.back().sound = 5;
                            }
                            else if (strcmp(lineSplitMTL[1], "wood") == 0)
                            {
                                fakeTextures.back().sound = 6;
                            }
                        }
                        else if (strcmp(lineSplitMTL[0], "particle") == 0 ||
                                 strcmp(lineSplitMTL[0], "\tparticle") == 0)
                        {
                            fakeTextures.back().particle = (char)round(std::stof(lineSplitMTL[1]));
                        }
                    }
                    free(lineSplitMTL);
                }
                fileMTL.close();
            }
        }
        free(lineSplit);
    }
    file.close();

    collisionModel->generateMinMaxValues();

    return collisionModel;
}

CollisionModel* ObjLoader::loadBinaryCollisionModel(std::string filePath, std::string fileName)
{
    std::list<FakeTexture> fakeTextures;
    std::vector<Vector3f> vertices;

    char currType = 0;
    char currSound = 0;
    char currParticle = 0;

    FILE* file = nullptr;
    int err = fopen_s(&file, (Global::pathToEXE + "res/" + filePath+fileName+".bincol").c_str(), "rb");
    if (file == nullptr || err != 0)
    {
        std::fprintf(stdout, "Error: Cannot load file '%s'\n", (Global::pathToEXE + "res/" + filePath+fileName+".bincol").c_str());
        return nullptr;
    }

    CollisionModel* collisionModel = new CollisionModel; INCR_NEW("CollisionModel");

    char fileType[4];
    fread(fileType, sizeof(char), 4, file);
    if (fileType[0] != 'c' || 
        fileType[1] != 'o' ||
        fileType[2] != 'l' ||
        fileType[3] != 0)
    {
        std::fprintf(stdout, "Error: File '%s' is not a valid .bincol file\n", (Global::pathToEXE + "res/" + filePath+fileName+".bincol").c_str());
        return collisionModel;
    }

    std::string mtlname = "";
    int mtllibLength;
    fread(&mtllibLength, sizeof(int), 1, file);
    for (int i = 0; i < mtllibLength; i++)
    {
        char nextChar;
        fread(&nextChar, sizeof(char), 1, file);
        mtlname = mtlname + nextChar;
    }

    {
        std::ifstream fileMTL(Global::pathToEXE + "res/" + filePath + mtlname);
        if (!fileMTL.is_open())
        {
            std::fprintf(stdout, "Error: Cannot load file '%s'\n", (Global::pathToEXE + "res/" + filePath + mtlname).c_str());
            fileMTL.close();
            fclose(file);
            return collisionModel;
        }

        std::string lineMTL;

        while (!fileMTL.eof())
        {
            getlineSafe(fileMTL, lineMTL);

            char lineBufMTL[256];
            memcpy(lineBufMTL, lineMTL.c_str(), lineMTL.size()+1);

            int splitLengthMTL = 0;
            char** lineSplitMTL = split(lineBufMTL, ' ', &splitLengthMTL);

            if (splitLengthMTL > 1)
            {
                if (strcmp(lineSplitMTL[0], "newmtl") == 0)
                {
                    FakeTexture fktex;

                    fktex.name = lineSplitMTL[1];
                    fakeTextures.push_back(fktex);
                }
                else if (strcmp(lineSplitMTL[0], "type") == 0 ||
                         strcmp(lineSplitMTL[0], "\ttype") == 0)
                {
                    if (strcmp(lineSplitMTL[1], "bounce") == 0)
                    {
                        fakeTextures.back().type = 1;
                    }
                    else if (strcmp(lineSplitMTL[1], "death") == 0)
                    {
                        fakeTextures.back().type = 2;
                    }
                }
                else if (strcmp(lineSplitMTL[0], "sound") == 0 ||
                         strcmp(lineSplitMTL[0], "\tsound") == 0)
                {
                    if (strcmp(lineSplitMTL[1], "concrete") == 0)
                    {
                        fakeTextures.back().sound = 0;
                    }
                    else if (strcmp(lineSplitMTL[1], "dirt") == 0)
                    {
                        fakeTextures.back().sound = 1;
                    }
                    else if (strcmp(lineSplitMTL[1], "grass") == 0)
                    {
                        fakeTextures.back().sound = 2;
                    }
                    else if (strcmp(lineSplitMTL[1], "metal") == 0)
                    {
                        fakeTextures.back().sound = 3;
                    }
                    else if (strcmp(lineSplitMTL[1], "snow") == 0)
                    {
                        fakeTextures.back().sound = 4;
                    }
                    else if (strcmp(lineSplitMTL[1], "water") == 0)
                    {
                        fakeTextures.back().sound = 5;
                    }
                    else if (strcmp(lineSplitMTL[1], "wood") == 0)
                    {
                        fakeTextures.back().sound = 6;
                    }
                }
                else if (strcmp(lineSplitMTL[0], "particle") == 0 ||
                         strcmp(lineSplitMTL[0], "\tparticle") == 0)
                {
                    fakeTextures.back().particle = (char)round(std::stof(lineSplitMTL[1]));
                }
            }
            free(lineSplitMTL);
        }
        fileMTL.close();
    }


    int numVertices;
    fread(&numVertices, sizeof(int), 1, file);
    vertices.reserve(numVertices);
    for (int i = 0; i < numVertices; i++)
    {
        float t[3];
        fread(t, sizeof(float), 3, file);

        Vector3f vertex(t[0], t[1], t[2]);
        vertices.push_back(vertex);
    }

    //int bytesPerIndV;
    //fread(&bytesPerIndV, sizeof(int), 1, file);

    int numMaterials;
    fread(&numMaterials, sizeof(int), 1, file);
    for (int m = 0; m < numMaterials; m++)
    {
        int matnameLength;
        fread(&matnameLength, sizeof(int), 1, file);
        std::string matname = "";
        for (int c = 0; c < matnameLength; c++)
        {
            char nextChar;
            fread(&nextChar, sizeof(char), 1, file);
            matname = matname + nextChar;
        }

        currType = 0;
        currSound = -1;
        currParticle = 0;

        for (FakeTexture dummy : fakeTextures)
        {
            if (dummy.name == matname)
            {
                currType = dummy.type;
                currSound = dummy.sound;
                currParticle = dummy.particle;
            }
        }

        std::vector<int> indices;
        int numFaces;
        fread(&numFaces, sizeof(int), 1, file);
        indices.reserve(numFaces*3);
        for (int i = 0; i < numFaces; i++)
        {
            //int f[3] = {0,0,0};
            //
            //fread(&f[0], bytesPerIndV, 1, file);
            //fread(&f[1], bytesPerIndV, 1, file);
            //fread(&f[2], bytesPerIndV, 1, file);

            int f[3];

            fread(&f[0], sizeof(int), 3, file);

            Triangle3D* tri = new Triangle3D(&vertices[f[0]-1], &vertices[f[1]-1], &vertices[f[2]-1], currType, currSound, currParticle); INCR_NEW("Triangle3D");

            collisionModel->triangles.push_back(tri);
        }
    }
    fclose(file);

    collisionModel->generateMinMaxValues();

    return collisionModel;
}

void ObjLoader::deleteUnusedMtl(std::unordered_map<std::string, ModelTexture>* mtlMap, std::vector<ModelTexture>* usedMtls)
{
    //for some extremely bizarre reason, I cannot make a set or a map on the stack in this specific function.
    // "trying to use deleted function" or something...
    //so we must do a slow O(n^2) operation here instead of linear...

    //iterate through loaded mtls, check if they are used
    std::unordered_map<std::string, ModelTexture>::iterator it;
    for (it = mtlMap->begin(); it != mtlMap->end(); it++)
    {
        bool foundTexture = false;
        for (int i = 0; i < (int)usedMtls->size(); i++)
        {
            ModelTexture tex = (*usedMtls)[i]; //get a copy of it
            if (tex.equalTo(&it->second))
            {
                foundTexture = true;
                break;
            }
        }

        if (!foundTexture)
        {
            ModelTexture texToDelete = it->second;
            texToDelete.deleteMe();
        }
    }
}
