#include <iostream>
#include <vector>
#include <unordered_set>

#include "modeltexture.hpp"
#include "../toolbox/maths.hpp"

std::unordered_set<ModelTexture*> ModelTexture::animatedTextureReferences;

ModelTexture::ModelTexture()
{
    shineDamper = 20.0f;
    reflectivity = 0.0f;
    hasTransparency = false;
    useFakeLighting = false;
    glowAmount = 0;
    isAnimated = false;
    animatedProgress = 0.0f;
    animationSpeed = 0.0f;
    currentImageIndex = 0;
    mixingType = 1;
    fogScale = 1.0f;
    renderOrder = 0;
}

ModelTexture::ModelTexture(ModelTexture* other)
{
    shineDamper         = other->shineDamper;
    reflectivity        = other->reflectivity;
    hasTransparency     = other->hasTransparency;
    useFakeLighting     = other->useFakeLighting;
    glowAmount          = other->glowAmount;
    isAnimated          = other->isAnimated;
    animatedProgress    = other->animatedProgress;
    animationSpeed      = other->animationSpeed;
    currentImageIndex   = other->currentImageIndex;
    mixingType          = other->mixingType;
    scrollX             = other->scrollX;
    scrollY             = other->scrollY;
    fogScale            = other->fogScale;
    renderOrder         = other->renderOrder;
}

bool ModelTexture::hasMultipleImages()
{
    return isAnimated;
}

void ModelTexture::deleteMe()
{
    ModelTexture::animatedTextureReferences.erase(this);
}

float ModelTexture::mixFactor()
{
    switch (mixingType)
    {
        case 1:
            return 0.0f;

        case 2:
            return animatedProgress;

        case 3:
            return 0.5f*(sinf(Maths::PI*(animatedProgress - 0.5f)) + 1);
    }

    return 0.0f;
}

void ModelTexture::addMeToAnimationsSetIfNeeded()
{
    if (isAnimated)
    {
        ModelTexture::animatedTextureReferences.insert(this);
    }
}

void ModelTexture::updateAnimations(float dt)
{
    for (ModelTexture* tex : ModelTexture::animatedTextureReferences)
    {
        tex->animatedProgress += tex->animationSpeed*dt;
        if (tex->animatedProgress >= 1.0f)
        {
            tex->animatedProgress-=1.0f;
        }
    }
}

bool ModelTexture::equalTo(ModelTexture* other)
{
    return (
        isAnimated      == other->isAnimated      &&
        shineDamper     == other->shineDamper     &&
        reflectivity    == other->reflectivity    &&
        scrollX         == other->scrollX         &&
        scrollY         == other->scrollY         &&
        glowAmount      == other->glowAmount      &&
        hasTransparency == other->hasTransparency &&
        useFakeLighting == other->useFakeLighting &&
        fogScale        == other->fogScale        &&
        mixingType      == other->mixingType      &&
        animationSpeed  == other->animationSpeed  &&
        renderOrder     == other->renderOrder);
}
