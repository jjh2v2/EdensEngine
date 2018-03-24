#include "Render/Light/DirectionalLight.h"

DirectionalLight::DirectionalLight(float startingAngle)
{
    mDirection = Vector3::Zero();
    mLightAngle = 0;

    SetLightAngle(MathHelper::Clamp(startingAngle, 0.0f, 360.0f));
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::SetDirection(Vector3 direction)
{
    mDirection = direction.Normalized();
}

void DirectionalLight::SetLightAngle(float angle)
{
    mLightAngle = angle;

    if (mLightAngle <= 90.0f)
    {
        mLightAngle = 270.0f;
    }

    float radians = mLightAngle * MathHelper::Radian();
    SetDirection(Vector3(sinf(radians), cosf(radians), 0.0f));
}

void DirectionalLight::FadeDayNight(float fadeAmount)
{
    SetLightAngle(mLightAngle - fadeAmount);
}