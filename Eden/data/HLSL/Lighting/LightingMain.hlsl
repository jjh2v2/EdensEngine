
struct VS_to_PS
{
    float4 positionViewport : SV_Position;
    float4 positionClip     : positionClip;
    float2 texCoord         : texCoord;
};

cbuffer MatrixBuffer
{
	matrix mCameraProj;
	matrix mCameraViewToLightProj;
	matrix mCameraViewInv;
};

cbuffer LightBuffer
{
	float4 skyColor;
    float4 groundColor;
	float4 lightDir;
	float4 lightColor;
	float lightIntensity;
	float ambientIntensity;
	float brdfspecular;
};


Texture2D gGBufferTextures[4];
StructuredBuffer<ShadowPartition> gPartitions;
Texture2DArray gShadowTextures[4];
TextureCube gEnvironmentMap;
Texture2D<float2> EnvBRDFLookupTexture;
SamplerState gShadowSampler;
SamplerState gEnvironmentSampler;


float4 LightingMainPixelShader(VS_to_PS input) : SV_Target
{
	float3 positionView = ComputePositionView(uint2(input.positionViewport.xy));
	float3 viewDir = normalize(positionView);
	
	LightingSurface surface = GetLightingSurfaceFromGBuffer(uint2(input.positionViewport.xy), input.texCoord, positionView);
	
	float3 texCoord = float3(0,0,0);
	float3 texCoordDX = float3(0,0,0);
	float3 texCoordDY = float3(0,0,0);
	float4 occluder = float4(0,0,0,0);
	ShadowPartition partition;
	
	if(positionView.z >= gPartitions[0].intervalBegin && positionView.z < gPartitions[0].intervalEnd)
	{
		partition = gPartitions[0];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[0].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[1].intervalBegin && positionView.z < gPartitions[1].intervalEnd)
	{
		partition = gPartitions[1];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[1].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[2].intervalBegin && positionView.z < gPartitions[2].intervalEnd)
	{
		partition = gPartitions[2];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[2].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[3].intervalBegin && positionView.z < gPartitions[3].intervalEnd)
	{
		partition = gPartitions[3];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[3].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else
	{
		discard;
	}
	
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
		float specMetallic = max(metallic, 0.03);
		
		float3 specularFactor = CalculateSchlickFresnelReflectance(viewDir, half, float3(specMetallic, specMetallic, specMetallic) * surfaceSpec) *
			  CalculateSmithGGXGeometryTerm(roughness, nDotL, nDotV) *
			  CalculateNormalDistributionTrowReitz(roughness, surface.normal, half) *
			  nDotL * lerp(1.0f, brdfspecular, metallic);
		
		float3 diffuseFactor = surface.albedo.rgb * nDotL * (1.0 - metallic);
		float shadowContrib = ShadowContribution(texCoord, texCoordDX, texCoordDY, saturate(texCoord.z), partition, occluder);
		
		lightingOutput.rgb += (diffuseFactor + specularFactor) * shadowContrib;
	}
	
	lightingOutput.rgb = ApplyFog(lightingOutput.rgb * pow(lightColor.rgb,2.2), length(positionView)/1000.0, viewDir);
	lightingOutput.rgb *= lightIntensity;
	
    return float4(lightingOutput, 1.0f);
}