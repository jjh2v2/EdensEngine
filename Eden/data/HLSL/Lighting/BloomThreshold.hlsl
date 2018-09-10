#include "../Common/ToneMapShaderCommon.hlsl"

Texture2D HDRTexture : register(t0);
Texture2D LuminanceTexture : register(t1);
SamplerState HDRTextureSampler : register(s0); //linear

cbuffer ThresholdBuffer : register(b0)
{
	float bloomThreshold;
    uint  toneMapMode;
};

struct BloomVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

BloomVertexOutput BloomThresholdVertexShader(uint vertexID : SV_VertexID)
{
    BloomVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 BloomThresholdPixelShader(BloomVertexOutput input) : SV_Target
{
    float3 color = HDRTexture.Sample(HDRTextureSampler, input.texCoord0).rgb;
    float  averageLuminance = GetAverageLuminance(LuminanceTexture);
    color = ToneMap(color, averageLuminance, bloomThreshold, toneMapMode);
	
    return float4(color, 1.0f);
}