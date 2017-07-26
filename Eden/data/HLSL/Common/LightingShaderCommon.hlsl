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

float CalculateGeometryTermSmithGGX(float roughness, float nDotL, float nDotV)
{
	float roughnessSquared = roughness * roughness;
	float viewGeoTerm  = nDotV + sqrt((nDotV - nDotV * roughnessSquared) * nDotV + roughnessSquared);
	float lightGeoTerm = nDotL + sqrt((nDotL - nDotL * roughnessSquared) * nDotL + roughnessSquared);
	
	return rcp( viewGeoTerm * lightGeoTerm );
}

int RoughnessToMipLevel(float roughness, int mipCount)
{
	return mipCount - 6 - 1.15 * log2(roughness);
}

float3 CalculateSpecularIBL(int mipLevels, float3 specularColor, float roughness, float nDotV, float3 reflection)
{
	float3 prefilteredColor = gEnvironmentMap.SampleLevel(gEnvironmentSampler, reflection, mipLevels - RoughnessToMipLevel(roughness, mipLevels)).rgb;
	prefilteredColor = pow(prefilteredColor, 2.2);
	float2 envBRDF = saturate(EnvBRDFLookupTexture.Sample(gEnvironmentSampler, saturate(float2(roughness, nDotV))));
	return prefilteredColor * (specularColor * envBRDF.x + envBRDF.y);
}

float3 ApplyFog(float3 pixelColor, float distance, float3 viewDir, float3 lightDir)
{
	float fogAmount = 1.0 - exp(-distance * 3.0);
    float sunAmount = max(dot(viewDir, lightDir), 0.0);
    float3 fogColor = lerp(pow(float3(0.5,0.6,0.7), 2.2), // bluish
                           pow(float3(1.0,0.9,0.7), 2.2), // yellowish
                           saturate(pow(sunAmount,8.0)));
	
	fogColor = lerp(pixelColor, fogColor, pow(fogAmount, 6.0) ;
	return fogColor;
}