//SDSM concept from http://visual-computing.intel-research.net/art/publications/sdsm/

struct ShadowPartition
{
    float3 scale; // Texture coordinate 0 - 1 space
    float  intervalBegin;
    float3 bias;
    float  intervalEnd;
};

struct ShadowPartitionUint
{
    uint3 scale;
    uint  intervalBegin;
    uint3 bias;
    uint  intervalEnd;
};

struct ShadowPartitionBoundUint
{
    uint3 minCoord;
    uint  padding;
    uint3 maxCoord;
    uint  padding2;
};

struct ShadowPartitionBoundFloat
{
    float3 minCoord;
    float  padding;
    float3 maxCoord;
    float  padding2;
};

float GetLogPartitionFromDepthRange(uint partitionIndex, uint numPartitions, float minDepth, float maxDepth)
{
    float depthResult = maxDepth;
    
    if (partitionIndex < numPartitions) 
    {
        float depthRatio = maxDepth / minDepth;
        float partitionExponent = float(partitionIndex) * (1.0f / float(numPartitions));
        depthResult = minDepth * pow(abs(depthRatio), partitionExponent); //abs to make hlsl compiler happy
    }
    
    return depthResult;
}

float2 GetEVSMExponents(ShadowPartition partition)
{
    float2 lightSpaceExponents = float2(800.0f, 100.0f);
    return min(lightSpaceExponents / partition.scale.zz, float2(42.0f, 42.0f));
}

float2 WarpDepth(float depth, float2 exponents)
{
    // Rescale depth to the -1 to 1 range
    depth = 2.0f * depth - 1.0f;
    float pos =  exp( exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return float2(pos, neg);
}

float ChebyshevUpperBound(float2 moments, float mean, float minVariance)
{
    // Compute variance
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);
    
    // Compute probabilistic upper bound
    float d = mean - moments.x;
    float pMax = variance / (variance + (d * d));
    
    // One-tailed Chebyshev
    return (mean <= moments.x ? 1.0f : pMax);
}

float GetShadowContribution(float3 texCoord, float3 texCoordDX, float3 texCoordDY, float depth, ShadowPartition partition, float4 occluder)
{
    float2 exponents = GetEVSMExponents(partition);
    float2 warpedDepth = WarpDepth(depth, exponents);
    
    // Derivative of warping at depth
    float2 depthScale = 0.0001f * exponents * warpedDepth;
    float2 minVariance = depthScale * depthScale;
    
    float posContrib = ChebyshevUpperBound(occluder.xz, warpedDepth.x, minVariance.x);
    float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y);
    
    return min(posContrib, negContrib);
}

void ComputePartitionScaleAndBias(ShadowPartitionBoundFloat bounds, float4 lightSpaceBorder, float4 maxScale, float dilationFactor, out float3 scale, out float3 bias)
{
    float3 minTexCoord = bounds.minCoord;
    float3 maxTexCoord = bounds.maxCoord;
        
    // Border gives space for softened edges
    minTexCoord -= lightSpaceBorder.xyz;
    maxTexCoord += lightSpaceBorder.xyz;
        
    scale = 1.0f / (maxTexCoord - minTexCoord);
    bias = -minTexCoord * scale;

    float oneMinusTwoFactor = 1.0f - 2.0f * dilationFactor;
    scale *= oneMinusTwoFactor;
    bias = dilationFactor + oneMinusTwoFactor * bias;
    
    // Clamp scale (but remain centered)
    float3 center = float3(0.5f, 0.5f, 0.5f);
    float3 clampedScale = min(scale, maxScale.xyz);
    bias = (clampedScale / scale) * (bias - center) + center;
    scale = clampedScale;
    
    if (scale.x < 0.0f) 
    {
        // empty partition
        scale = asfloat(0x7F7FFFFF).xxx;
        bias = scale;
    }
}