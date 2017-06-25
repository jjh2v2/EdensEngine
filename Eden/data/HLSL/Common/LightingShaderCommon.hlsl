#include "../Common/ShaderCommon.hlsl"

struct LightingSurface
{
    float4 albedo;
	float3 normal;
	float4 material;
    float3 lightTexCoord;        
    float3 lightTexCoordDX;
    float3 lightTexCoordDY;
};

float3 ViewPositionToLightTexCoord(float3 positionView, matrix cameraViewToLightProj)
{
    float4 positionLight = mul(float4(positionView, 1.0f), cameraViewToLightProj);
    float3 texCoord = (positionLight.xyz / positionLight.w) * float3(0.5f, -0.5f, 1.0f) + float3(0.5f, 0.5f, 0.0f);
    return texCoord;
}

LightingSurface GetLightingSurfaceFromGBuffer(uint2 coords, float2 texCoord, float3 positionView)
{
    float2 bufferDimensions;
    gGBufferTextures[0].GetDimensions(bufferDimensions.x, bufferDimensions.y);
    
    float2 screenPixelOffset = float2(2.0f, -2.0f) / bufferDimensions;
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    
    LightingSurface data;
	float4 normal = gGBufferTextures[0][coords];
	data.albedo = gGBufferTextures[1][coords];
	data.material = gGBufferTextures[2][coords];
    data.normal = DecodeSphereMap(normal.xy);
    
	float2 positionScreenX = positionScreen + float2(screenPixelOffset.x, 0.0f);
    float2 positionScreenY = positionScreen + float2(0.0f, screenPixelOffset.y);
	float3 positionViewDX = GetViewPosition(positionScreenX, positionView.z + normal.z) - positionView;
    float3 positionViewDY = GetViewPosition(positionScreenY, positionView.z + normal.w) - positionView;
    data.lightTexCoord = ViewPositionToLightTexCoord(positionView);
    data.lightTexCoordDX = (ViewPositionToLightTexCoord(positionView + 2.0 * positionViewDX) - data.lightTexCoord) / 2.0;
    data.lightTexCoordDY = (ViewPositionToLightTexCoord(positionView + 2.0 * positionViewDY) - data.lightTexCoord) / 2.0;
    
    return data;
}

float3 CalculateFresnelShlick(float3 specular, float3 viewDir, float3 half)
{
	return specular + (1.0 - specular) * pow(1.0 - dot(viewDir, half), 5.0);
}

float CalculateNormalDistributionTW(float roughness, float nDotH)
{
	float PI = 3.14159265f;
	float roughnessSquared = roughness * roughness;
	float denom = nDotH * nDotH * (roughnessSquared - 1.0) + 1.0;
	return (roughnessSquared) / ((PI * denom * denom) + 0.00001); //the 0.00001 prevents light artifacts from close-to-zero values
}

float CalculateGeometryTermSmithGGX(float roughness, float nDotL, float nDotV)
{
	float roughnessSquared = roughness * roughness;
	float viewGeoTerm = nDotV + sqrt( (nDotV - nDotV * roughnessSquared) * nDotV + roughnessSquared );
	float lightGeoTerm = nDotL + sqrt( (nDotL - nDotL * roughnessSquared) * nDotL + roughnessSquared );
	
	return rcp( viewGeoTerm * lightGeoTerm );
}

int RoughnessToMipLevel(float roughness, int mipCount)
{
	return mipCount - 6 - 1.15 * log2(roughness);
}

float3 CalculateSpecularIBL(int MipLevels, float3 SpecularColor, float Roughness, float nDotV, float3 Reflection)
{
	float3 PrefilteredColor = gEnvironmentMap.SampleLevel(gEnvironmentSampler, Reflection, MipLevels - RoughnessToMipLevel(Roughness, MipLevels)).rgb;
	PrefilteredColor = pow(PrefilteredColor, 2.2);
	float2 EnvBRDF = saturate(EnvBRDFLookupTexture.Sample(gEnvironmentSampler, saturate(float2(Roughness, nDotV))));
	return PrefilteredColor * (SpecularColor * EnvBRDF.x + EnvBRDF.y);
}

float3 ApplyFog(float3 pixelColor, float distance, float3 viewDir, float3 lightDir)
{
	float b = 3.0f;
	float fogAmount = 1.0 - exp( -distance * b );
    float sunAmount = max(dot(viewDir, lightDir), 0.0);
    float3 fogColor = lerp(pow(float3(0.5,0.6,0.7), 2.2), // bluish
                           pow(float3(1.0,0.9,0.7), 2.2), // yellowish
                           saturate(pow(sunAmount,8.0)));
	
	fogColor = lerp(pixelColor, fogColor, pow(fogAmount, 6.0) ;
	return fogColor;
}