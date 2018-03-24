StructuredBuffer<ShadowPartition> ShadowPartitionsR : register(t0);

struct ShadowVertexInput
{
    float4 position : POSITION;
};

cbuffer MatrixBuffer
{
	matrix poLightWorldViewProj;
    uint   poPartitionIndex;
};

float4 ShadowMapVertexShader(ShadowVertexInput input) : SV_Position
{
	input.position.w = 1.0f;
    float4 position = mul(input.position, lightWorldViewProj);
    
    ShadowPartition partition = ShadowPartitionsR[partitionIndex];
    position.xy *= partition.scale.xy;
    position.x  += (2.0f * partition.bias.x + partition.scale.x - 1.0f);
    position.y  -= (2.0f * partition.bias.y + partition.scale.y - 1.0f);
    position.z  += EPSILON;
    position.z   = position.z * partition.scale.z + partition.bias.z;
    
    return position;
}