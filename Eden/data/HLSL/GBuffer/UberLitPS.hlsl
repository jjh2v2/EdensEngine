#include "Consts/PixelShaderConsts.hlsl"
#include "Consts/SharedShaderConsts.hlsl"

float2 EncodeSphereMap(float3 n)
{
    float oneMinusZ = 1.0f - n.z;
    float p = sqrt(n.x * n.x + n.y * n.y + oneMinusZ * oneMinusZ);
	p = max(p, 0.00001);

    return n.xy * rcp(p) * 0.5f + 0.5f;
}

struct GBuffer
{
    float4 normals : SV_Target0;
    float4 albedo  : SV_Target1;
	float4 specular : SV_Target2;
};

GBuffer UberLitPixelShader(VS_to_PS input) : SV_TARGET
{					  
    float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, input.texCoord0 * float2(tileX, tileY));
	
	textureColor.rgb = pow(textureColor.rgb, 2.2);	//gamma correction
	
	clip(textureColor.a < 0.1f ? -1 : 1);
	
	float4 specular = float4(specularColor.rgb, 0.0);
	
#ifdef USE_NORMAL_MAP
	float4 bumpMap = NormalMap.Sample(NormalMapSampler, input.texCoord0 * float2(tileX, tileY));
	bumpMap = (bumpMap * 2.0f) - 1.0f;
	
	float3 viewSpaceBinormal = cross( input.deferredNormal.xyz, input.tangent );
	
	float3x3 texSpace = float3x3(input.tangent, viewSpaceBinormal, input.deferredNormal);
	
	float3 defNormal = normalize(mul(bumpMap, texSpace));
	
	
#ifdef USE_SPECULAR_MAP
	float4 specularIntensity = SpecularMap.Sample(SpecularMapSampler, input.texCoord0 * float2(tileX, tileY)); 
	specular = float4(specularColor.rgb * specularIntensity.rgb, saturate(specularPower/400.0f));
#endif

#else
	float3 defNormal = normalize(input.deferredNormal);
#endif
	
	GBuffer result;

	result.specular = specular * materialIntensity;
	result.albedo = textureColor * diffuseColor * materialIntensity;
	
#ifdef USE_ROUGHMETAL_MAP
	float2 roughMetal = RoughMetalMap.Sample(RoughMetalMapSampler, input.texCoord0 * float2(tileX, tileY)).rg;
	result.albedo.a = max(roughMetal.r, 0.05);
	result.specular.a = roughMetal.g;
#else
	result.albedo.a = max(roughness, 0.05);
	result.specular.a = metalness;
#endif

	result.specular.r = materialIntensity;
	result.normals = float4(EncodeSphereMap(defNormal),
                            ddx_coarse(input.deferredPositionView.z),
                            ddy_coarse(input.deferredPositionView.z));
    return result;
}