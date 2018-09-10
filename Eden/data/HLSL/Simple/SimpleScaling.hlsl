Texture2D ScalingTexture : register(t0);
SamplerState ScalingSampler : register(s0); //linear

struct ScalingVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

ScalingVertexOutput SimpleScalingVertexShader(uint vertexID : SV_VertexID)
{
    ScalingVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 SimpleScalingPixelShader(ScalingVertexOutput input) : SV_Target
{
    return ScalingTexture.Sample(ScalingSampler, input.texCoord0);
}