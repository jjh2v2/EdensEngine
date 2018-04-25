#include "../Common/EnvironmentShaderCommon.hlsl"

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
	float2 clipCoords = (float2(dispatchThreadID.xy) / pfSourceDimensions.xy) * 2.0f - 1.0f;
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