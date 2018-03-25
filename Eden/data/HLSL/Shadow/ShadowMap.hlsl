#include "../Common/ShaderCommon.hlsl"
#include "../Common/ShadowShaderCommon.hlsl"

struct ShadowVertexInput
{
    float4 position : POSITION;
};

StructuredBuffer<ShadowPartition> ShadowPartitionsR : register(t0);

cbuffer PartitionBuffer : register(b0)
{
    uint pfPartitionIndex;
};

cbuffer ShadowMatrixBuffer : register(b1)
{
    matrix poLightWorldViewProj;
};

float4 ShadowMapVertexShader(ShadowVertexInput input) : SV_Position
{
	input.position.w = 1.0f;
    float4 position = mul(input.position, poLightWorldViewProj);
    
    ShadowPartition partition = ShadowPartitionsR[pfPartitionIndex];
    position.xy *= partition.scale.xy;
    position.x  += (2.0f * partition.bias.x + partition.scale.x - 1.0f);
    position.y  -= (2.0f * partition.bias.y + partition.scale.y - 1.0f);
    position.z  += EPSILON;
    position.z   = position.z * partition.scale.z + partition.bias.z;
    
    return position;
}