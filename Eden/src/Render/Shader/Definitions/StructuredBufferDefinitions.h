#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Vector/Vector4.h"
#include "Core/Vector/Vector3.h"
#include "Core/Vector/Vector2.h"

struct SDSMPartition
{
    Vector3 scale;
    Vector3 bias;
    float intervalBegin;
    float intervalEnd;
};

struct SDSMBounds
{
    Vector3 minCoord;
    Vector3 maxCoord;
    Vector2 padding;
};