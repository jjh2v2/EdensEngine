#include "../Common/ToneMapShaderCommon.hlsl"

Texture2D HDRTexture : register(t0);
Texture2D LuminanceTexture : register(t1);
Texture2D BloomTexture : register(t2);
SamplerState ToneMapSampler : register(s0); //linear

cbuffer ToneMapBuffer : register(b0)
{
	float bloomMagnitude;
    uint  toneMapMode;
};

struct ToneMapVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

ToneMapVertexOutput ToneMapCompositeVertexShader(uint vertexID : SV_VertexID)
{
    ToneMapVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 ToneMapCompositePixelShader(ToneMapVertexOutput input) : SV_Target
{
    float3 color = HDRTexture.Sample(ToneMapSampler, input.texCoord0).rgb;
    float  averageLuminance = GetAverageLuminance(LuminanceTexture);
    color = ToneMap(color, averageLuminance, 0, toneMapMode);
    
	float3 bloom = BloomTexture.Sample(ToneMapSampler, input.texCoord0).rgb * bloomMagnitude;
    
    return float4(color + bloom, 1.0f);
}
