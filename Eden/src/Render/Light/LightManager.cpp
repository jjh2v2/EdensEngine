#include "Render/Light/LightManager.h"

LightManager::LightManager()
{
    mSunLight = new DirectionalLight(220.0f);
}

LightManager::~LightManager()
{
    delete mSunLight;
}

void LightManager::Update(float deltaTime)
{

}