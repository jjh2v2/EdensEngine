#include "../Common/ShadowShaderCommon.hlsl"
#define SHADOWMAP_SAMPLE_COUNT 4

cbuffer PartitionBuffer : register(b0)
{
    uint pfPartitionIndex;
};

Texture2DMS<float, SHADOWMAP_SAMPLE_COUNT> ShadowMapTextureMS : register(t0);
StructuredBuffer<ShadowPartition> ShadowPartitionsR : register(t1);

struct VertexOutput
{
    float4 positionViewport : SV_POSITION;
};

VertexOutput ShadowMapToEVSMVertexShader(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    output.positionViewport = float4(grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);
    
    return output;
}

float4 ShadowMapToEVSMPixelShader(VertexOutput input) : SV_Target
{
    float sampleWeight = 1.0f / float(SHADOWMAP_SAMPLE_COUNT);
    int2  shadowMapCoords = int2(input.positionViewport.xy);
    float2 exponents = GetEVSMExponents(ShadowPartitionsR[partitionIndex]);
    float4 average = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    [unroll] 
    for (uint i = 0; i < SHADOWMAP_SAMPLE_COUNT; i++) 
    {
        float depth = ShadowMapTextureMS.Load(shadowMapCoords, i);
        float2 warpedDepth = WarpDepth(depth, exponents);
        average += sampleWeight * float4(warpedDepth.xy, warpedDepth.xy * warpedDepth.xy);
    }
    
    return average;
}