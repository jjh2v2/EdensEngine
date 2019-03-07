#include "../Common/ShaderCommon.hlsl"

float3 CalculateSchlickFresnelReflectance(float vDotH, float3 specular)
{
    return specular + (1.0 - specular) * pow(1.0 - vDotH, 5.0);
}

float CalculateNormalDistributionGGX(float roughness, float nDotH)
{
    float roughnessSquared = roughness * roughness;
    float f = (nDotH * roughnessSquared - nDotH) * nDotH + 1.0;
    return roughnessSquared / (PI * f * f + EPSILON);
}

float CalculateSmithGGXGeometryTerm(float roughness, float nDotL, float nDotV)
{
    float roughnessSquared = roughness * roughness;
    float lightTerm = nDotV * sqrt((nDotL - nDotL * roughnessSquared) * nDotL + roughnessSquared);
    float viewTerm = nDotL * sqrt((nDotV - nDotV * roughnessSquared) * nDotV + roughnessSquared);
    
    return 0.5 / (lightTerm + viewTerm);
}

float LambertDiffuse()
{
    return 1.0 / PI;
}

float BurleyDiffuse(float linearRoughness, float NoV, float NoL, float LoH) 
{
    float f90 = 0.5 + 2.0 * linearRoughness * LoH * LoH;
    float lightScatter = CalculateSchlickFresnelReflectance(NoL,1).r;
    float viewScatter  = CalculateSchlickFresnelReflectance(NoV,1).r;
    return lightScatter * viewScatter * (1.0 / PI);
}

float3 CalculateDiffuseIBL(TextureCube envMap, SamplerState envSampler, int mipLevels, float3 reflection)
{
    return pow(abs(envMap.SampleLevel(envSampler, reflection, mipLevels).rgb), 2.2);
}

float3 CalculateSpecularIBL(int mipLevels, float3 specularColor, float roughness, float nDotV, float3 reflection, TextureCube envMap, Texture2D<float2> envLookup, SamplerState envSampler)
{
	float3 prefilteredColor = envMap.SampleLevel(envSampler, reflection, mipLevels - RoughnessToMipLevel(roughness, mipLevels)).rgb;
	prefilteredColor = pow(abs(prefilteredColor), 2.2);
	float2 envBRDF = saturate(envLookup.Sample(envSampler, saturate(float2(roughness, nDotV))));
	return prefilteredColor * (specularColor * envBRDF.x + envBRDF.y);
}