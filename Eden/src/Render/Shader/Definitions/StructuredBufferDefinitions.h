#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Vector/Vector4.h"
#include "Core/Vector/Vector3.h"
#include "Core/Vector/Vector2.h"

struct SDSMPartition
{
    Vector3 scale;
    float intervalBegin;
    Vector3 bias;
    float intervalEnd;
};

struct SDSMBounds
{
    Vector3 minCoord;
    float padding;
    Vector3 maxCoord;
    float padding2;
};