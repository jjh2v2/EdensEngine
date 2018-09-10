#include "../Common/ShaderCommon.hlsl"

Texture2D BlurTexture : register(t0);
SamplerState BlurSampler : register(s0); //point

cbuffer BlurBuffer : register(b0)
{
	float blurSigma;
};

struct BlurVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

float CalcGaussianWeight(int sampleDist)
{
	float g = 1.0f / sqrt(2.0f * PI * blurSigma * blurSigma);  
	return (g * exp(-(sampleDist * sampleDist) / (2 * blurSigma * blurSigma)));
}

float4 Blur(float2 texCoord, float2 texScale)
{
    uint2 textureDimensions;
    BlurTexture.GetDimensions(textureDimensions.x, textureDimensions.y);
    
    float4 color = 0;
    for (int i = -6; i < 6; i++)
    {   
		float weight = CalcGaussianWeight(i);
		float2 newTexCoord = texCoord;
		newTexCoord += (i / float2(textureDimensions.x, textureDimensions.y)) * texScale;
		float4 sample = BlurTexture.Sample(BlurSampler, newTexCoord);
		color += sample * weight;
    }

    return color;
}

BlurVertexOutput SimpleBlurVertexShader(uint vertexID : SV_VertexID)
{
    BlurVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 SimpleBlurHPixelShader(BlurVertexOutput input): SV_Target
{
	return Blur(input.texCoord0, float2(1, 0));
}

float4 SimpleBlurVPixelShader(BlurVertexOutput input): SV_Target
{
	return Blur(input.texCoord0, float2(0, 1));
}
