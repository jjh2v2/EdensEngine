#include "../Common/ShaderCommon.hlsl"

float3 CalculateFresnelShlick(float3 specular, float3 viewDir, float3 half)
{
	return specular + (1.0 - specular) * pow(1.0 - dot(viewDir, half), 5.0);
}

float CalculateNormalDistributionTW(float roughness, float nDotH)
{
	float roughnessSquared = roughness * roughness;
	float denom = nDotH * nDotH * (roughnessSquared - 1.0) + 1.0;
	return (roughnessSquared) / ((PI * denom * denom) + 0.00001); //the 0.00001 prevents light artifacts from close-to-zero values
}

float CalculateGeometryTermSmithGGXAlt(float roughness, float nDotL, float nDotV)
{
	float roughnessSquared = roughness * roughness;
	float viewGeoTerm  = nDotV + sqrt((nDotV - nDotV * roughnessSquared) * nDotV + roughnessSquared);
	float lightGeoTerm = nDotL + sqrt((nDotL - nDotL * roughnessSquared) * nDotL + roughnessSquared);
	
	return rcp( viewGeoTerm * lightGeoTerm );
}

float3 CalculateSpecularIBL(int mipLevels, float3 specularColor, float roughness, float nDotV, float3 reflection, TextureCube envMap, Texture2D<float2> envLookup, SamplerState envSampler)
{
	float3 prefilteredColor = envMap.SampleLevel(envSampler, reflection, mipLevels - RoughnessToMipLevel(roughness, mipLevels)).rgb;
	prefilteredColor = pow(abs(prefilteredColor), 2.2);
	float2 envBRDF = saturate(envLookup.Sample(envSampler, saturate(float2(roughness, nDotV))));
	return prefilteredColor * (specularColor * envBRDF.x + envBRDF.y);
}

float G1Smith(float k, float nDotV)
{
	return nDotV / (nDotV * (1.0 - k) + k);
}

float CalculateGeometryTermSmithGGX(float roughness, float nDotL, float nDotV)
{
	float r2 = (roughness + 1) * (roughness + 1);
	float k = r2 / 8.0;
	return G1Smith(k, nDotV) * G1Smith(k, nDotL);
}