struct GBuffer
{
    float4 normals;
    float4 albedo;
	float4 specular;
};

struct SurfaceData
{
    float3 normal;               // View space normal
    float4 albedo;
    float3 lightTexCoord;        // Texture coordinates and depth in light space, [0, 1]
    float3 lightTexCoordDX;      // Screen space partial derivatives
    float3 lightTexCoordDY;      // of light space texture coordinates.
	float4 specular;
};

struct Partition
{
    float intervalBegin;
    float intervalEnd;
    
    // These are given in texture coordinate [0, 1] space
    float3 scale;
    float3 bias;
};


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
	float4 lightDir;
	float4 lightColor;
	float lightIntensity;
	float ambientIntensity;
	float brdfspecular;
};

cbuffer SkyColorBuffer
{
    float4 skyColor;
    float4 groundColor;
};

Texture2D gGBufferTextures[4];
StructuredBuffer<Partition> gPartitions;
Texture2DArray gShadowTextures[4];
TextureCube gEnvironmentMap;
Texture2D<float2> EnvBRDFLookupTexture;
SamplerState gShadowSampler;
SamplerState gEnvironmentSampler;

float2 GetEVSMExponents(Partition partition)
{
    float2 lightSpaceExponents = float2(800.0f, 100.0f);//float2(mUI.positiveExponent, mUI.negativeExponent);
    
    // Make sure exponents say consistent in light space regardless of partition
    // scaling. This prevents the exponentials from ever getting too rediculous
    // and maintains consistency across partitions.
    // Clamp to maximum range of fp32 to prevent overflow/underflow
    return min(lightSpaceExponents / partition.scale.zz, float2(42.0f, 42.0f));
}

float2 WarpDepth(float depth, float2 exponents)
{
    // Rescale depth into [-1, 1]
    depth = 2.0f * depth - 1.0f;
    float pos =  exp( exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return float2(pos, neg);
}

float3 ComputePositionViewFromZ(float2 positionScreen, float viewSpaceZ)
{
    float2 screenSpaceRay = float2(positionScreen.x / mCameraProj._11, positionScreen.y / mCameraProj._22);
    
    float3 positionView;
    positionView.z = viewSpaceZ;
    // Solve the two projection equations
    positionView.xy = screenSpaceRay.xy * positionView.z;
    
    return positionView;
}

float3 ProjectIntoLightTexCoord(float3 positionView)
{
    float4 positionLight = mul(float4(positionView, 1.0f), mCameraViewToLightProj);
    float3 texCoord = (positionLight.xyz / positionLight.w) * float3(0.5f, -0.5f, 1.0f) + float3(0.5f, 0.5f, 0.0f);
    return texCoord;
}


float ChebyshevUpperBound(float2 moments, float mean, float minVariance)
{
    // Compute variance
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance);
    
    // Compute probabilistic upper bound
    float d = mean - moments.x;
    float pMax = variance / (variance + (d * d));
    
    // One-tailed Chebyshev
    return (mean <= moments.x ? 1.0f : pMax);
}

float ShadowContribution(float3 texCoord, float3 texCoordDX, float3 texCoordDY, float depth,
                         Partition partition, float4 occluder)
{
    float2 exponents = GetEVSMExponents(partition);
    float2 warpedDepth = WarpDepth(depth, exponents);
    
    // Derivative of warping at depth
    // TODO: Parameterize min depth stddev
    float2 depthScale = 0.0001f * exponents * warpedDepth;
    float2 minVariance = depthScale * depthScale;
    
    float posContrib = ChebyshevUpperBound(occluder.xz, warpedDepth.x, minVariance.x);
    float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y);
    
    float shadowContrib = posContrib;
    shadowContrib = min(shadowContrib, negContrib);
    
    return shadowContrib;
}

float3 ComputePositionView(uint2 coords)
{
	float zBuffer   = gGBufferTextures[3][coords].x;
	float2 gbufferDim;
    gGBufferTextures[0].GetDimensions(gbufferDim.x, gbufferDim.y);
    
    float2 screenPixelOffset = float2(2.0f, -2.0f) / gbufferDim;
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
	float viewSpaceZ = mCameraProj._43 / (zBuffer - mCameraProj._33);
	
	return ComputePositionViewFromZ(positionScreen , viewSpaceZ);
}

SurfaceData ComputeSurfaceDataFromGBuffer(uint2 coords, float2 texCoord, float3 positionView)
{
    GBuffer rawData;
    rawData.normals = gGBufferTextures[0][coords];
    rawData.albedo  = gGBufferTextures[1][coords];
	rawData.specular = gGBufferTextures[2][coords];
    
    float2 gbufferDim;
    gGBufferTextures[0].GetDimensions(gbufferDim.x, gbufferDim.y);
    
    float2 screenPixelOffset = float2(2.0f, -2.0f) / gbufferDim;
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    
    SurfaceData data;
	data.specular = rawData.specular;
    data.normal = DecodeSphereMap(rawData.normals.xy);
    data.albedo = rawData.albedo;
	
    // Solve for light space position and screen-space derivatives
    float deltaPixels = 2.0f;
	float2 positionScreenX = positionScreen + float2(screenPixelOffset.x, 0.0f);
    float2 positionScreenY = positionScreen + float2(0.0f, screenPixelOffset.y);
	float3 positionViewDX = ComputePositionViewFromZ(positionScreenX, positionView.z + rawData.normals.z) - positionView;
    float3 positionViewDY = ComputePositionViewFromZ(positionScreenY, positionView.z + rawData.normals.w) - positionView;
    data.lightTexCoord   = (ProjectIntoLightTexCoord(positionView));
    data.lightTexCoordDX = (ProjectIntoLightTexCoord(positionView + deltaPixels * positionViewDX) - data.lightTexCoord) / deltaPixels;
    data.lightTexCoordDY = (ProjectIntoLightTexCoord(positionView + deltaPixels * positionViewDY) - data.lightTexCoord) / deltaPixels;
    
    return data;
}

float3 CalculateFresnelShlick(float3 specular, float3 viewDir, float3 half)
{
	return specular + (1.0 - specular) * pow(1.0 - dot(viewDir, half), 5.0);
}

float CalculateNormalDistributionTW(float roughness, float NoH)
{
	float PI = 3.14159265f;
	float roughnessSquared = roughness * roughness;
	float denom = NoH * NoH * (roughnessSquared - 1.0) + 1.0;
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

float3 ApproximateSpecularIBL(int MipLevels, float3 SpecularColor, float Roughness, float3 N, float3 V)
{
	float NoV = abs(dot(N, V));
	float3 R = reflect(-V, N);
	R = mul(R, (float3x3)mCameraViewInv);
	
	float3 PrefilteredColor = gEnvironmentMap.SampleLevel(gEnvironmentSampler, R, MipLevels - RoughnessToMipLevel(Roughness, MipLevels)).rgb;
	PrefilteredColor = pow(PrefilteredColor, 2.2);
	float2 EnvBRDF = saturate(EnvBRDFLookupTexture.Sample(gEnvironmentSampler, saturate(float2(Roughness, NoV))));
	return PrefilteredColor * (SpecularColor * EnvBRDF.x + EnvBRDF.y);
}

float3 ApplyFog(float3 pixelColor, float distance, float3 viewDir)
{
	float b = 3.0f;
	float fogAmount = 1.0 - exp( -distance*b );
    float sunAmount = max( dot( viewDir, lightDir.xyz ), 0.0 );
    float3  fogColor  = lerp( pow(float3(0.5,0.6,0.7), 2.2), // bluish
                           pow(float3(1.0,0.9,0.7), 2.2), // yellowish
                           saturate(pow(sunAmount,8.0)) );
	
	fogColor = lerp( pixelColor, fogColor, pow(fogAmount, 6.0) );
	return fogColor;
}

float4 LightingMainPixelShader(VS_to_PS input) : SV_Target
{
    float3 lit = float3(0.0f, 0.0f, 0.0f);
	float3 positionView = ComputePositionView(uint2(input.positionViewport.xy));
	
	uint partitionIndex = 0;
	float4 occluder = float4(0,0,0,0);
	
	SurfaceData surface = ComputeSurfaceDataFromGBuffer(uint2(input.positionViewport.xy), input.texCoord, positionView);
	
	Partition partition; 
	
	float3 texCoord = float3(0,0,0);
	float3 texCoordDX = float3(0,0,0);
	float3 texCoordDY = float3(0,0,0);
	
	if(positionView.z >= gPartitions[0].intervalBegin && positionView.z < gPartitions[0].intervalEnd)
	{
		partitionIndex = 0;
		partition = gPartitions[0];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[0].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[1].intervalBegin && positionView.z < gPartitions[1].intervalEnd)
	{
		partitionIndex = 1;
		partition = gPartitions[1];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[1].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[2].intervalBegin && positionView.z < gPartitions[2].intervalEnd)
	{
		partitionIndex = 2;
		partition = gPartitions[2];
		texCoord = surface.lightTexCoord.xyz * partition.scale.xyz + partition.bias.xyz;
	    texCoordDX = surface.lightTexCoordDX.xyz * partition.scale.xyz;
	    texCoordDY = surface.lightTexCoordDY.xyz * partition.scale.xyz;
		occluder = gShadowTextures[2].SampleGrad(gShadowSampler, float3(texCoord.xy, 0), texCoordDX.xy, texCoordDY.xy);
	}
	else if(positionView.z >= gPartitions[3].intervalBegin && positionView.z < gPartitions[3].intervalEnd)
	{
		partitionIndex = 3;
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
	
	float nDotL = saturate(dot(-lightDir.xyz, surface.normal));
	
	float roughness = surface.albedo.a;
	float roughness *= roughness;
	float metallic = surface.specular.a;
	
    float3 skyDirection = float3(0.0f,1.0f,0.0f);
    float ndotSky = (dot(surface.normal, skyDirection) * 0.5f ) + 0.5f;
    float3 ambientLight = ambientIntensity * lerp(pow(groundColor.rgb,2.2), pow(skyColor.rgb,2.2), ndotSky);
	
    float3 ambientFactor = ambientLight.rgb * lerp(surface.albedo.rgb, float3(0.05, 0.05, 0.05), metallic); 
     
    lit += ambientFactor;

	float3 viewDir = normalize(positionView);
	
	lit += ApproximateSpecularIBL(11, lerp(float3(0.0f, 0.0f, 0.0f), surface.albedo.rgb, metallic), roughness, surface.normal, viewDir); 
	
	if(nDotL > 0.0f)
	{
		float3 surfaceSpec = lerp(float3(1.0, 1.0, 1.0), surface.albedo.rgb, metallic);
		float specMetallic = max(metallic, 0.03);
		float shadowContrib = 1.0;

		float depth = clamp(texCoord.z, 0.0f, 1.0f);

		shadowContrib = ShadowContribution(texCoord, texCoordDX, texCoordDY, depth, partition, occluder);
		
		float3 half = normalize(viewDir + lightDir.xyz);
		float nDotV = saturate(dot(surface.normal, viewDir));
		
		float3 diffuseFactor = surface.albedo.rgb * nDotL * shadowContrib * (1.0 - metallic);
		
		float3 specularFactor = CalculateSchlickFresnelReflectance(viewDir, half, float3(specMetallic, specMetallic, specMetallic) * surfaceSpec) *
			  CalculateSmithGGXGeometryTerm(roughness, nDotL, nDotV) *
			  CalculateNormalDistributionTrowReitz(roughness, surface.normal, half)
			  * shadowContrib * nDotL * lerp(1.0f, brdfspecular, metallic);
		
		lit.rgb += diffuseFactor + specularFactor;
	}
	lit.rgb = ApplyFog(lit.rgb * pow(lightColor.rgb,2.2), length(positionView)/1000.0, viewDir);
	
	lit.rgb *= lightIntensity;
	
    return float4(lit, 1.0f);
}