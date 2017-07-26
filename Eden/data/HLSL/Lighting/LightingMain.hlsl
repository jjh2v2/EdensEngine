#include "../Common/LightingMainShaderCommon.hlsl"
#include "../Common/LightingShaderCommon.hlsl"

LightingMainVertexOutput LightingMainVertexShader(uint vertexID : SV_VertexID)
{
    LightingMainVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);

    return output;
}

float4 LightingMainPixelShader(LightingMainVertexOutput input) : SV_Target
{
	float3 positionView = GetViewPosition(uint2(input.position.xy));
	LightingSurface surface = GetLightingSurfaceFromGBuffer(uint2(input.position.xy), input.texCoord0, positionView);
	
	float3 shadowTexCoord = float3(0,0,0);
	float3 shadowTexCoordDX = float3(0,0,0);
	float3 shadowTexCoordDY = float3(0,0,0);
	float4 shadowOccluder = float4(0,0,0,0);
	float  shadowMultiplier = 0.0;
	
	ShadowPartition partition;
	
	if(positionView.z >= gPartitions[0].intervalBegin && positionView.z < gPartitions[0].intervalEnd)
	{
		partition = gPartitions[0];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = gShadowTextures[0].SampleGrad(gShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[1].intervalBegin && positionView.z < gPartitions[1].intervalEnd)
	{
		partition = gPartitions[1];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = gShadowTextures[1].SampleGrad(gShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[2].intervalBegin && positionView.z < gPartitions[2].intervalEnd)
	{
		partition = gPartitions[2];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = gShadowTextures[2].SampleGrad(gShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[3].intervalBegin && positionView.z < gPartitions[3].intervalEnd)
	{
		partition = gPartitions[3];
		shadowTexCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    shadowTexCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    shadowTexCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		shadowOccluder = gShadowTextures[3].SampleGrad(gShadowSampler, float3(shadowTexCoord.xy, 0), shadowTexCoordDX.xy, shadowTexCoordDY.xy);
	}
	else
	{
		shadowMultiplier = 1.0;
	}
	
	float3 viewDir = normalize(positionView);
	float3 reflection = reflect(-viewDir, surface.normal);
	reflection = mul(reflection, (float3x3)mCameraViewInv);
	
	float nDotL = saturate(dot(-lightDir.xyz, surface.normal));
	float nDotV = saturate(dot(surface.normal, viewDir));
    float nDotSky = (dot(surface.normal, float3(0.0f, 1.0f, 0.0f)) * 0.5f ) + 0.5f;
    float3 ambientLight = ambientIntensity * lerp(pow(groundColor.rgb,2.2), pow(skyColor.rgb,2.2), ndotSky);
	
	float roughness = surface.albedo.a * surface.albedo.a;
	float metallic = surface.specular.a;
	float3 lightingOutput = float3(0.0f, 0.0f, 0.0f);
    lightingOutput += ambientLight.rgb * lerp(surface.albedo.rgb, float3(0.05, 0.05, 0.05), metallic);  //hemispheric ambient
	lightingOutput += CalculateSpecularIBL(11, lerp(float3(0.0f, 0.0f, 0.0f), surface.albedo.rgb, metallic), roughness, nDotV, reflection); 
	
	if(nDotL > 0.0f)
	{
		float3 half = normalize(viewDir + lightDir.xyz);
		float3 surfaceSpec = lerp(float3(1.0, 1.0, 1.0), surface.albedo.rgb, metallic);
		float  specMetallic = max(metallic, 0.03);
		
		float3 specularFactor = CalculateSchlickFresnelReflectance(viewDir, half, float3(specMetallic, specMetallic, specMetallic) * surfaceSpec) *
			  CalculateSmithGGXGeometryTerm(roughness, nDotL, nDotV) *
			  CalculateNormalDistributionTrowReitz(roughness, surface.normal, half) *
			  nDotL * lerp(1.0f, brdfspecular, metallic);
		
		float3 diffuseFactor = surface.albedo.rgb * nDotL * (1.0 - metallic);
		
		if(shadowMultiplier < 1.0)
		{
			shadowMultiplier = ShadowContribution(shadowTexCoord, shadowTexCoordDX, shadowTexCoordDY, saturate(shadowTexCoord.z), partition, shadowOccluder);
		}
		
		lightingOutput.rgb += (diffuseFactor + specularFactor) * shadowContrib;
	}
	
	lightingOutput.rgb = ApplyFog(lightingOutput.rgb * pow(lightColor.rgb,2.2), length(positionView)/1000.0, viewDir);
	lightingOutput.rgb *= lightIntensity;
	
    return float4(lightingOutput, 1.0f);
}