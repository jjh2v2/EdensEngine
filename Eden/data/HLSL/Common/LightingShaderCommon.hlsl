#include "../Common/ShaderCommon.hlsl"

float3 CalculateSchlickFresnelReflectance(float3 viewDir, float3 half, float3 specular)
{
	return specular + (1.0 - specular) * pow(1.0 - (dot(half, viewDir)), 5.0);
}

float CalculateSmithGGXGeometryTerm(float roughness, float nDotL, float nDotV)
{
	float roughnessActual = roughness * roughness;
	float viewGeoTerm = nDotV + sqrt( (nDotV - nDotV * roughnessActual) * nDotV + roughnessActual );
	float lightGeoTerm = nDotL + sqrt( (nDotL - nDotL * roughnessActual) * nDotL + roughnessActual );
	
	return rcp( viewGeoTerm * lightGeoTerm );
}

float CalculateNormalDistributionTrowReitz(float roughness, float3 surfaceNormal, float3 microfacetNormal)
{
	float roughnessActual = roughness * roughness;
	return pow(roughnessActual, 2.0) / (PI * pow( pow(dot(surfaceNormal, microfacetNormal), 2.0) 
									   * (pow(roughnessActual, 2.0) - 1.0) + 1.0 , 2.0) + 0.0000001);
}

float3 CalculateSpecularIBL(int mipLevels, float3 specularColor, float roughness, float3 normal, float3 viewDir, float3 reflection, TextureCube envMap, Texture2D<float2> envLookup, SamplerState envSampler)
{
    float nDotV = abs(dot(normal, viewDir));
	float3 prefilteredColor = envMap.SampleLevel(envSampler, reflection, mipLevels - RoughnessToMipLevel(roughness, mipLevels)).rgb;
	prefilteredColor = pow(abs(prefilteredColor), 2.2);
	float2 envBRDF = saturate(envLookup.Sample(envSampler, saturate(float2(roughness, nDotV))));
	return prefilteredColor * (specularColor * envBRDF.x + envBRDF.y);
}