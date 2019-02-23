#include "../Common/LightingShaderCommon.hlsl"

struct VertexInput
{
    float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

struct HullInput
{
    float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

struct DomainInput
{
	float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

struct PixelInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float4 positionView : TEXCOORD0;
    float4 texCoord0    : TEXCOORD1;
};

struct TessellationPatch
{
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;
};

cbuffer WaterBuffer : register(b0)
{
    matrix modelMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    float4 lightDirection;
    float  tessellationFactor;
    float  time;
};

Texture2D WaterHeightMap1 : register(t0);
Texture2D WaterHeightMap2 : register(t1);
Texture2D WaterNormalMap  : register(t2);
SamplerState WaterSampler : register(s0);

HullInput WaterVertexShader(VertexInput input)
{
    HullInput output;
    output.position = input.position;
	output.texCoord0 = input.texCoord0;
    
    return output;
}

TessellationPatch WaterTessellation(InputPatch<HullInput, 3> inputPatch, uint patchId : SV_PrimitiveID)
{
    TessellationPatch output;
	
	output.edges[0] = output.edges[1] = output.edges[2] = tessellationFactor;
    output.inside = tessellationFactor;
               
    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[maxtessfactor(15.0)]
[patchconstantfunc("WaterTessellation")]
DomainInput WaterHullShader(InputPatch<HullInput, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    DomainInput output;

    output.position = patch[pointId].position;
    output.texCoord0 = patch[pointId].texCoord0;

    return output;
}

float CalculateWaveHeight(float3 wavePosition)
{
    float wavelength = 0.8;
    float frequency = 2.0 / wavelength;
    float amplitude = 0.7;
    float speed = 1.2;
    float phaseConstant = speed * frequency;
    float2 direction = normalize(float2(1, 0));
    float rad = dot(direction, wavePosition.xz) * frequency + time * phaseConstant;
    
    return amplitude * sin(rad);
}

struct WaveResult
{
    float3 position;
    float3 binormal;
    float3 tangent;
};

WaveResult CalculateWavePosition2(float3 wavePosition, float edgeDampen)
{
    WaveResult result;
    
    float steepness = 0.5;
    float L = 0.8;
    float wi = 2.0 / L;
    float A = 0.7;
    float s = 1.2;
    float p = s * wi;
    float3 d = float3(1, 0, 0);
    float Qi = steepness / (A * wi);
    
    float rad = wi * dot(d.xz, wavePosition.xz) + time * p;
    float sinR = sin(rad);
    float cosR = cos(rad);
    
    result.position.x = wavePosition.x + Qi * A * d.x * cosR;
    result.position.z = wavePosition.z + Qi * A * d.z * cosR;
    result.position.y = A * sinR;
    
    float WA = wi * A;
    float radN = wi * dot(d, result.position) + time * p;
    float sinN = sin(radN);
    float cosN = cos(radN);
    
    result.binormal.x = 1 - (Qi * d.x * d.x * WA * sinN);
    result.binormal.z = -1 * (Qi * d.x * d.z * WA * sinN);
    result.binormal.y = d.x * WA * cosN;
    
    result.tangent.x = -1 * (Qi * d.x * d.z * WA * sinN);
    result.tangent.z = 1 - (Qi * d.z * d.z * WA * sinN);
    result.tangent.y = d.z * WA * cosN;
    
    return result;
}


[domain("tri")]
PixelInput WaterDomainShader(TessellationPatch input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<DomainInput, 3> patch)
{
    PixelInput output;
    output.position = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    output.texCoord0 = uvwCoord.x * patch[0].texCoord0 + uvwCoord.y * patch[1].texCoord0 + uvwCoord.z * patch[2].texCoord0;
    
    float dampening = 1.0 - saturate(abs(output.texCoord0.x - 0.5) / 0.5);
    dampening *= 1.0 - saturate(abs(output.texCoord0.y - 0.5) / 0.5);
    
    WaveResult waveResult = CalculateWavePosition2(output.position.xyz, dampening);
    output.position.xyz = waveResult.position;
    output.position.w = 1.0f;
    output.position = mul(output.position, modelMatrix);
    output.positionView = mul(output.position, viewMatrix);
    output.position = mul(output.positionView, projectionMatrix);
    
    output.normal = cross(waveResult.tangent, waveResult.binormal);
    output.normal = normalize(mul(output.normal, (float3x3)modelMatrix));
	output.normal = normalize(mul(float4(output.normal, 0.0f), viewMatrix).xyz);
    
    return output;
}

float4 WaterPixelShader(PixelInput input) : SV_TARGET
{
    float3 color = float3(0.24, 0.98, 0.96);
    
    float3 resultColor = color * saturate(dot(input.normal, -lightDirection.xyz));
    
    return float4(resultColor, 1.0);
}