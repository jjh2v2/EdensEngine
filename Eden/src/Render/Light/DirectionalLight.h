#pragma once

#include "Core/Vector/Vector3.h"

class DirectionalLight
{
public:
    DirectionalLight(float startingAngle);
    ~DirectionalLight();

    void SetLightAngle(float angle);
    void FadeDayNight(float fadeAmount);
    Vector3 GetDirection() { return mDirection; }

private:
    void SetDirection(Vector3 direction);

    Vector3 mDirection;
    float mLightAngle;
};