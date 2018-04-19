#include "../Common/ShaderCommon.hlsl"
#define ENV_MAP_SAMPLES 1024
#define FILTER_COMPUTE_DIMENSION 8

TextureCube  EnvironmentMap : register(t0);
SamplerState EnvironmentMapSampler : register(s0);
RWTexture2DArray<float4> FilteredCubemap : register(u0);

cbuffer FilterBuffer : register(b0)
{
    float4 pfUpDir;
	float4 pfForwardDir;
	float2 pfSourceDimensions;
	float  pfMipLevel;
	float  pfMipCount;
};

float RadicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N) 
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 xi, float roughness, float3 normal)
{
	float a = roughness * roughness;
	float phi = 2 * PI * xi.x;
	float cosTheta = sqrt((1 - xi.y) / (1 + (a*a - 1) * xi.y));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);

	float3 h;
	h.x = sinTheta * cos(phi);
	h.y = sinTheta * sin(phi);
	h.z = cosTheta;

	float3 upVector = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tangentX = normalize(cross(upVector, normal));
	float3 tangentY = cross(normal, tangentX);
	// Tangent to world space
	return tangentX * h.x + tangentY * h.y + normal * h.z;
}

float3 PrefilterEnvMap(TextureCube envMap, SamplerState envSampler, float roughness, float3 normal)
{
	float3 prefilteredColor = 0;
	float totalWeight = 0.0;

	for (uint i = 0; i < ENV_MAP_SAMPLES; i++)
	{
		float2 xi = Hammersley(i, ENV_MAP_SAMPLES);
		float3 h = ImportanceSampleGGX(xi, roughness, normal);
		float3 l = 2 * dot(normal, h) * h - normal;
		float nDotL = saturate(dot(normal, l));

		if (nDotL > 0)
		{
			prefilteredColor += pow(abs(envMap.SampleLevel(envSampler, l, 0).rgb), 2.2f) * nDotL;
			totalWeight += nDotL;
		}
	}

	return prefilteredColor / totalWeight;
}


[numthreads(FILTER_COMPUTE_DIMENSION, FILTER_COMPUTE_DIMENSION, 1)]
void FilterEnvironmentMap(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	float2 clipCoords = (float2(dispatchThreadID.xy) / float2(pfSourceDimensions.xy)) * 2.0f - 1.0f;
	float3 right = cross(pfUpDir.xyz, pfForwardDir.xyz);
	float3 normal = pfForwardDir.xyz;
	normal += clipCoords.x * right - clipCoords.y * pfUpDir.xyz;
	normal = normalize(normal);

	float3 color = PrefilterEnvMap(EnvironmentMap, EnvironmentMapSampler, MipLevelToRoughness(pfMipLevel, pfMipCount), normal);

	if (dispatchThreadID.x < uint(pfSourceDimensions.x) && dispatchThreadID.y < uint(pfSourceDimensions.y)) //possibly early out instead?
    {
		FilteredCubemap[uint3(dispatchThreadID.xy, 0)] = float4(color, 1.0f);
    }
}