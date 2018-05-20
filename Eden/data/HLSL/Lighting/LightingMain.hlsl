#include "../Common/LightingMainShaderCommon.hlsl"

LightingMainVertexOutput LightingMainVertexShader(uint vertexID : SV_VertexID)
{
    LightingMainVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);

    return output;
}

float4 LightingMainPixelShader(LightingMainVertexOutput input) : SV_Target
{
	LightingSurface surface = GetLightingSurfaceFromGBuffer(uint2(input.position.xy));
    
    if(surface.zDepth < 0.00001)
    {
        discard;
    }
    
	float3 shadowTexCoord = float3(0,0,0);
	float3 shadowTexCoordDX = float3(0,0,0);
	float3 shadowTexCoordDY = float3(0,0,0);
	float4 shadowOccluder = float4(0,0,0,0);
	float  shadowMultiplier = 0.0;
	
	ShadowPartition partition;
    partition.scale = float3(0,0,0); 
    partition.bias = float3(0,0,0);
    partition.intervalBegin = 0;
    partition.intervalEnd = 0;
	
	if(surface.positionView.z >= ShadowPartitions[0].intervalBegin && surface.positionView.z < ShadowPartitions[0].intervalEnd)
	{
		partition = ShadowPartitions[0];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = ShadowTextures[0].SampleGrad(ShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else if(surface.positionView.z >= ShadowPartitions[1].intervalBegin && surface.positionView.z < ShadowPartitions[1].intervalEnd)
	{
		partition = ShadowPartitions[1];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = ShadowTextures[1].SampleGrad(ShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else if(surface.positionView.z >= ShadowPartitions[2].intervalBegin && surface.positionView.z < ShadowPartitions[2].intervalEnd)
	{
		partition = ShadowPartitions[2];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = ShadowTextures[2].SampleGrad(ShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else if(surface.positionView.z >= ShadowPartitions[3].intervalBegin && surface.positionView.z < ShadowPartitions[3].intervalEnd)
	{
		partition = ShadowPartitions[3];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = ShadowTextures[3].SampleGrad(ShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else
	{
		shadowMultiplier = 1.0;
	}
	
	float3 viewDir = normalize(surface.positionView);
	float3 reflection = reflect(-viewDir, surface.normal);
	reflection = mul(reflection, (float3x3)pfViewInvMatrix);
	
	float nDotL = saturate(dot(-pfLightDir.xyz, surface.normal));
    float nDotSky = (dot(surface.normal, float3(0.0f, 1.0f, 0.0f)) * 0.5f ) + 0.5f;
    float3 ambientLight = pfAmbientIntensity * lerp(pow(abs(pfGroundColor.rgb),2.2), pow(abs(pfSkyColor.rgb),2.2), nDotSky);
	
	float roughness = surface.material.r; //* surface.material.r; //TDA: need to square this?
	float metallic = surface.material.g;
	float3 lightingOutput = float3(0.0f, 0.0f, 0.0f);
    lightingOutput += ambientLight.rgb * lerp(surface.albedo.rgb, float3(0.05, 0.05, 0.05), metallic);
	lightingOutput += CalculateSpecularIBL(pfSpecularIBLMipLevels, lerp(float3(0.0f, 0.0f, 0.0f), surface.albedo.rgb, metallic), roughness, surface.normal, viewDir, reflection, EnvironmentMap, EnvBRDFLookupTexture, EnvironmentSampler); 
	
	if(nDotL > 0.0f)
	{
		float3 half = normalize(viewDir + pfLightDir.xyz);
		float3 surfaceSpec = lerp(float3(1.0, 1.0, 1.0), surface.albedo.rgb, metallic);
		float  specMetallic = max(metallic, 0.03);
		
        float nDotV = saturate(dot(surface.normal, viewDir));
		
        float3 specularFactor = CalculateSchlickFresnelReflectance(viewDir, half, float3(specMetallic, specMetallic, specMetallic) * surfaceSpec) *
			  CalculateSmithGGXGeometryTerm(roughness, nDotL, nDotV) *
			  CalculateNormalDistributionTrowReitz(roughness, surface.normal, half) *
			  nDotL * lerp(1.0f, pfBRDFspecular, metallic);
        
		float3 diffuseFactor = surface.albedo.rgb * nDotL * (1.0 - metallic);
		
		if(shadowMultiplier < 1.0)
		{
			shadowMultiplier = GetShadowContribution(shadowTexCoord, shadowTexCoordDX, shadowTexCoordDY, saturate(shadowTexCoord.z), partition, shadowOccluder);
		}
		
		lightingOutput.rgb += (diffuseFactor + specularFactor) * pfLightIntensity * pfLightColor.rgb * shadowMultiplier;
	}
	
    return float4(lightingOutput * surface.material.b, 1.0f);
}