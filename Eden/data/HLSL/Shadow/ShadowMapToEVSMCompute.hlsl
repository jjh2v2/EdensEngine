#include "../Common/ShadowShaderCommon.hlsl"
#define SHADOWMAP_SAMPLE_COUNT 4

cbuffer PartitionBuffer : register(b0)
{
    uint pfPartitionIndex;
};

Texture2DMS<float, SHADOWMAP_SAMPLE_COUNT> ShadowMapTextureMS : register(t0);
StructuredBuffer<ShadowPartition> ShadowPartitionsR : register(t1);
RWTexture2D<float4> EVSMTexture : register(u0);

[numthreads(8, 8, 1)]
void ShadowMapToEVSMCompute(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float sampleWeight = 1.0f / float(SHADOWMAP_SAMPLE_COUNT);
    float2 exponents = GetEVSMExponents(ShadowPartitionsR[pfPartitionIndex]);
    float4 average = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    [unroll] 
    for (uint i = 0; i < SHADOWMAP_SAMPLE_COUNT; i++) 
    {
        float depth = ShadowMapTextureMS.Load(dispatchThreadID.xy, i);
        float2 warpedDepth = WarpDepth(depth, exponents);
        average += sampleWeight * float4(warpedDepth.xy, warpedDepth.xy * warpedDepth.xy);
    }
    
    EVSMTexture[dispatchThreadID.xy] = average;
}