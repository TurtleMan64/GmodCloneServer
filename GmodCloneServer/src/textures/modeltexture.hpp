#ifndef MODELTEXTURES_H
#define MODELTEXTURES_H

#include "../toolbox/vector.hpp"
#include <vector>
#include <unordered_set>

class ModelTexture
{
private:
    //animation 
    bool isAnimated; //has more than 1 image
    char currentImageIndex; //current index of the animation
    float animatedProgress; //progress to the next image in the animation

    //for use of updating the animation values
    static std::unordered_set<ModelTexture*> animatedTextureReferences;

public:
    float shineDamper;
    float reflectivity;
    float scrollX;
    float scrollY;
    float glowAmount;
    bool hasTransparency;
    bool useFakeLighting;
    float fogScale;
    int mixingType; //interpolation. 1 = binary, 2 = linear, 3 = sinusoid
    float animationSpeed; //delta per second
    char renderOrder; //0 = rendered first (default), 1 = second, 2 = third, 3 = fifth + transparent (no depth testing), 4 = fourth + no depth writing

    ModelTexture();

    ModelTexture(ModelTexture* other);

    bool hasMultipleImages();

    //how much the 2nd image should be mixed with the first (for animations)
    float mixFactor();

    //deletes all of the texture Ids out of gpu memory
    void deleteMe();

    void addMeToAnimationsSetIfNeeded();

    //updates all of the textures animation progress by dt
    static void updateAnimations(float dt);

    bool equalTo(ModelTexture* other);
};
#endif
