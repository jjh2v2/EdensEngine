#pragma once

#include "Render/Light/DirectionalLight.h"

class LightManager
{
public:
    LightManager();
    ~LightManager();

    void Update(float deltaTime);
    DirectionalLight *GetSunLight() { return mSunLight; }

private:
    DirectionalLight *mSunLight;
};