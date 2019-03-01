#include "../Common/LightingShaderCommon.hlsl"
#include "../Common/ShadowShaderCommon.hlsl"

struct LightingMainVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

struct LightingSurface
{
    float4 albedo;
    float4 material;
	float3 normal;
    float3 positionView;
    float  zDepth;
};

cbuffer LightingPassBuffer : register(b0)
{
	matrix pfViewMatrix;
    matrix pfProjectionMatrix;
	matrix pfViewInvMatrix;
    float4 pfSkyColor;
    float4 pfGroundColor;
	float4 pfLightDir;
	float4 pfLightColor;
    float2 pfBufferDimensions;
	float  pfLightIntensity;
	float  pfAmbientIntensity;
	float  pfBRDFspecular;
    uint   pfSpecularIBLMipLevels;
};

Texture2D GBufferTextures[4] : register(t0);
TextureCube EnvironmentMap : register(t4);
Texture2D<float2> EnvBRDFLookupTexture : register(t5);
Texture2D<float> ShadowTexture : register(t6);
SamplerState EnvironmentSampler : register(s0);
SamplerState ShadowLinearSampler : register(s1);

LightingSurface GetLightingSurface(uint2 coords)
{
    LightingSurface surface;
	float4 normal    = GBufferTextures[1][coords];
	surface.albedo   = GBufferTextures[0][coords];
	surface.material = GBufferTextures[2][coords];
	surface.zDepth   = GBufferTextures[3][coords].r;
    surface.normal   = DecodeSphereMap(normal.xy);
    surface.positionView = GetViewPosition(coords, pfBufferDimensions, surface.zDepth, pfProjectionMatrix);
    
    return surface;
}

LightingMainVertexOutput LightingMainVertexShader(uint vertexID : SV_VertexID)
{
    LightingMainVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float4 LightingMainPixelShader(LightingMainVertexOutput input) : SV_Target
{
	LightingSurface surface = GetLightingSurface(uint2(input.position.xy));
    
    if(surface.zDepth < 0.00001)
    {
        discard; //skybox
    }
    
	float3 viewDir = normalize(surface.positionView);
	float3 reflection = reflect(viewDir, surface.normal);
	reflection = mul(reflection, (float3x3)pfViewInvMatrix);
	
	float nDotL = saturate(dot(-pfLightDir.xyz, surface.normal));
    float nDotSky = (dot(surface.normal, float3(0.0f, 1.0f, 0.0f)) * 0.5f ) + 0.5f;
    float3 ambientLight = pfAmbientIntensity * lerp(pow(abs(pfGroundColor.rgb),2.2), pow(abs(pfSkyColor.rgb),2.2), nDotSky);
	
	float roughness = surface.material.r;
	float metallic = surface.material.g;
	float3 lightingOutput = float3(0.0f, 0.0f, 0.0f);
    lightingOutput += ambientLight.rgb * lerp(surface.albedo.rgb, float3(0.05, 0.05, 0.05), metallic);
	lightingOutput += CalculateSpecularIBL(pfSpecularIBLMipLevels, lerp(float3(0.0f, 0.0f, 0.0f), surface.albedo.rgb, metallic), roughness, surface.normal, viewDir, reflection, EnvironmentMap, EnvBRDFLookupTexture, EnvironmentSampler); 
	
	if(nDotL > 0.0f)
	{
		float3 half = normalize(viewDir + pfLightDir.xyz);
		float3 surfaceSpec = lerp(float3(1.0, 1.0, 1.0), surface.albedo.rgb, metallic);
		float specMetallic = max(metallic, 0.03);
        float nDotV = saturate(dot(surface.normal, viewDir));
		
        float3 specularFactor = CalculateSchlickFresnelReflectance(viewDir, half, float3(specMetallic, specMetallic, specMetallic) * surfaceSpec) *
			  CalculateSmithGGXGeometryTerm(roughness, nDotL, nDotV) *
			  CalculateNormalDistributionTrowReitz(roughness, surface.normal, half) *
			  nDotL * lerp(1.0f, pfBRDFspecular, metallic);
        
		float3 diffuseFactor = surface.albedo.rgb * nDotL * (1.0 - metallic);
		float shadowMultiplier = ShadowTexture.Sample(ShadowLinearSampler, input.texCoord0);
		
		lightingOutput.rgb += (diffuseFactor + specularFactor) * pfLightIntensity * pfLightColor.rgb * shadowMultiplier;
	}
	
    return float4(lightingOutput * surface.material.b, 1.0f);
}