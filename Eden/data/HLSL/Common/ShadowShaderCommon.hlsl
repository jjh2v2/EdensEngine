//SDSM concept from http://visual-computing.intel-research.net/art/publications/sdsm/

struct ShadowPartition
{
    float3 scale; // Texture coordinate 0 - 1 space
    float3 bias;
    float intervalBegin;
    float intervalEnd;
};

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