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
    float4 position       : SV_POSITION;
    float3 normal         : NORMAL;
    float3 tangent        : TANGENT;
    float3 binormal       : BINORMAL;
    float4 positionView   : TEXCOORD0;
    float4 texCoord0      : TEXCOORD1;
    float4 screenPosition : TEXCOORD2;
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

Texture2D WaterNormalMap1 : register(t0);
Texture2D WaterNormalMap2 : register(t1);
Texture2D HDRMap          : register(t2);
Texture2D DepthMap        : register(t3);
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
    float3 normal;
    float3 binormal;
    float3 tangent;
};

struct Wave
{
    float3 direction;
    float steepness;
    float waveLength;
    float amplitude;
    float speed;
};

WaveResult CalculateWave(Wave wave, float3 wavePosition, float edgeDampen, float numWaves)
{
    WaveResult result;
    
    float frequency = sqrt(9.8 * ((2 * 3.14159) / wave.waveLength)); //2.0 / L;
    float phaseConstant = wave.speed * frequency;
    float qi = wave.steepness / (wave.amplitude * frequency * numWaves);
    float rad = frequency * dot(wave.direction.xz, wavePosition.xz) + time * phaseConstant;
    float sinR = sin(rad);
    float cosR = cos(rad);
    
    result.position.x = wavePosition.x + qi * wave.amplitude * wave.direction.x * cosR * edgeDampen;
    result.position.z = wavePosition.z + qi * wave.amplitude * wave.direction.z * cosR * edgeDampen;
    result.position.y = wave.amplitude * sinR * edgeDampen;
    
    float waFactor = frequency * wave.amplitude;
    float radN = frequency * dot(wave.direction, result.position) + time * phaseConstant;
    float sinN = sin(radN);
    float cosN = cos(radN);
    
    result.binormal.x = 1 - (qi * wave.direction.x * wave.direction.x * waFactor * sinN);
    result.binormal.z = -1 * (qi * wave.direction.x * wave.direction.z * waFactor * sinN);
    result.binormal.y = wave.direction.x * waFactor * cosN;
    
    result.tangent.x = -1 * (qi * wave.direction.x * wave.direction.z * waFactor * sinN);
    result.tangent.z = 1 - (qi * wave.direction.z * wave.direction.z * waFactor * sinN);
    result.tangent.y = wave.direction.z * waFactor * cosN;
    
    result.normal.x = -1 * (wave.direction.x * waFactor * cosN);
    result.normal.z = -1 * (wave.direction.z * waFactor * cosN);
    result.normal.y = 1 - (qi * waFactor * sinN);
    
    result.normal = lerp(result.normal, float3(0, 1, 0), 0);
    
    result.binormal = normalize(result.binormal);
    result.tangent = normalize(result.tangent);
    result.normal = normalize(result.normal);
    
    return result;
}


[domain("tri")]
PixelInput WaterDomainShader(TessellationPatch input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<DomainInput, 3> patch)
{
    PixelInput output;
    output.position = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    output.texCoord0 = uvwCoord.x * patch[0].texCoord0 + uvwCoord.y * patch[1].texCoord0 + uvwCoord.z * patch[2].texCoord0;
    
    uint numWaves = 2;
    Wave waves[2];
    waves[0].direction = normalize(float3(0.3, 0, -0.7));
    waves[0].steepness = 0.99;
    waves[0].waveLength = 8.75;
    waves[0].amplitude = 0.065;
    waves[0].speed = 1.0;
    
    waves[1].direction = normalize(float3(0.5, 0, -0.2));
    waves[1].steepness = 0.99;
    waves[1].waveLength = 6.1;
    waves[1].amplitude = 0.12;
    waves[1].speed = 1.3;
    
    float dampening = 1.0 - pow(saturate(abs(output.texCoord0.x - 0.5) / 0.5), 5.0);
    dampening *= 1.0 - pow(saturate(abs(output.texCoord0.y - 0.5) / 0.5), 5.0);
    dampening = 1;
    
    WaveResult finalWaveResult;
    finalWaveResult.position = float3(0,0,0);
    finalWaveResult.normal = float3(0,0,0);
    finalWaveResult.tangent = float3(0,0,0);
    finalWaveResult.binormal = float3(0,0,0);
    
    for(uint waveId = 0; waveId < numWaves; waveId++)
    {
        WaveResult waveResult = CalculateWave(waves[waveId], output.position.xyz, dampening, numWaves);
        finalWaveResult.position += waveResult.position;
        finalWaveResult.normal += waveResult.normal;
        finalWaveResult.tangent += waveResult.tangent;
        finalWaveResult.binormal += waveResult.binormal;
    }
    
    finalWaveResult.position -= output.position.xyz * (numWaves - 1);
    finalWaveResult.normal = normalize(finalWaveResult.normal);
    finalWaveResult.tangent = normalize(finalWaveResult.tangent);
    finalWaveResult.binormal = normalize(finalWaveResult.binormal);
    
    output.position.xyz = finalWaveResult.position;
    output.position.w = 1.0f;
    output.position = mul(output.position, modelMatrix);
    output.positionView = mul(output.position, viewMatrix);
    output.position = mul(output.positionView, projectionMatrix);
    output.screenPosition = output.position;
    
    output.normal = normalize(mul(finalWaveResult.normal, (float3x3)modelMatrix));
	output.normal = normalize(mul(float4(output.normal, 0.0f), viewMatrix).xyz);
    output.tangent = normalize(mul(finalWaveResult.tangent, (float3x3)modelMatrix));
	output.tangent = normalize(mul(float4(output.tangent, 0.0f), viewMatrix).xyz);
    output.binormal = normalize(mul(finalWaveResult.binormal, (float3x3)modelMatrix));
	output.binormal = normalize(mul(float4(output.binormal, 0.0f), viewMatrix).xyz);
    
    return output;
}

float4 WaterPixelShader(PixelInput input) : SV_TARGET
{
    float3 color = float3(0.24, 0.98, 0.96) * 0.6;
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    input.binormal = normalize(input.binormal);
    
    float2 texCoords = input.texCoord0.xy + time * float2(1.0, 0.0) * 0.1;
    float2 texCoords2 = input.texCoord0.xy + time * float2(0.0, 1.0) * 0.1;
    float2 hdrCoords = ((float2(input.screenPosition.x, -input.screenPosition.y) / input.screenPosition.w) * 0.5) + 0.5;
    
    float4 normalMap = (WaterNormalMap1.Sample(WaterSampler, texCoords) * 2.0) - 1.0;
    float4 normalMap2 = (WaterNormalMap2.Sample(WaterSampler, texCoords2) * 2.0) - 1.0;
    float3x3 texSpace = float3x3(input.tangent, input.binormal, input.normal);
	float3 finalNormal = normalize(mul(normalMap.xyz, texSpace));
    finalNormal += normalize(mul(normalMap2.xyz, texSpace));
    finalNormal = normalize(finalNormal);
        
    float3 resultColor = color * HDRMap.Sample(WaterSampler, hdrCoords + finalNormal.xz * 0.01).rgb; //Use different sampler, point //* saturate(dot(input.normal, -lightDirection.xyz));
    
    float3 viewDir = normalize(input.positionView.xyz);
    float3 half = normalize(viewDir + lightDirection.xyz);
    float roughness = 0.03;
    float nDotL = saturate(dot(-lightDirection.xyz, finalNormal));
    float nDotV = saturate(dot(finalNormal, viewDir));
    
    float3 specularFactor = CalculateSchlickFresnelReflectance(viewDir, half, float3(0.025, 0.025, 0.025)) *
			  CalculateSmithGGXGeometryTerm(roughness, nDotL, nDotV) *
			  CalculateNormalDistributionTrowReitz(roughness, finalNormal, half) *
			  nDotL;
    
    return float4(resultColor + specularFactor, 1.0);
}