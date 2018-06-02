#include "../Common/ShaderCommon.hlsl"
#define BLUR_HALF_SAMPLE_COUNT 7

Texture2D BlurSource : register(t0);
SamplerState BlurSampler : register(s0);

cbuffer BloomBlurBuffer : register(b0)
{
    float2 pfBlurInputDimensions;
    float  pfBlurSigma;
};

struct VertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

float GetGaussianWeight(int sampleOffset)
{
    float blur2Squared = pfBlurSigma * pfBlurSigma * 2.0;
    float gaussian = 1.0f / sqrt(blur2Squared * PI);
    return (gaussian * exp(-(sampleOffset * sampleOffset) / blur2Squared));
}

float4 WeightedBlur(float2 texCoord, float2 blurDirection)
{
    float4 color = 0;
    
    for(int i = -BLUR_HALF_SAMPLE_COUNT; i < BLUR_HALF_SAMPLE_COUNT; i++)
    {
        float4 sample = BlurSource.Sample(BlurSampler, texCoord + (i / pfBlurInputDimensions) * blurDirection);
        color += sample * GetGaussianWeight(i);
    }

    return color;
}

VertexOutput BloomBlurVertexShader(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 BloomBlurHorizontalPixelShader(VertexOutput input) : SV_Target
{
    return WeightedBlur(input.texCoord0, float2(1, 0));
}

float4 BloomBlurVerticalPixelShader(VertexOutput input) : SV_Target
{
    return WeightedBlur(input.texCoord0, float2(0, 1));
}