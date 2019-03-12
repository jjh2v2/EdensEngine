#include "../Common/LightingShaderCommon.hlsl"

struct VertexInput
{
    float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

struct HullInput
{
    float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

struct DomainInput
{
	float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

struct PixelInput
{
    float4 position             : SV_POSITION;
    float3 normal               : NORMAL;
    float3 tangent              : TANGENT;
    float3 binormal             : BINORMAL;
    float4 positionView         : TEXCOORD0;
    float4 texCoord0            : TEXCOORD1;
    float4 screenPosition       : TEXCOORD2;
    float4 positionWorld        : TEXCOORD3;
    float4 worldNormalAndHeight : TEXCOORD4;
};

struct TessellationPatch
{
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;
};

cbuffer WaterBuffer : register(b0)
{
    matrix modelMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix viewInverseMatrix;
    matrix projectionInverseMatrix;
    matrix viewProjInvMatrix;
    float4 lightDirection;
    float4 waterSurfaceColor;
    float4 waterRefractionColor;
    float4 ssrSettings;              //step size, max steps forward, max steps back, foreground reduction factor
    float4 normalMapScroll;
    float2 normalMapScrollSpeed;
    float  refractionDistortionFactor;
    float  roughness;
    float  reflectance;
    float  specIntensity;
    float  tessellationFactor;
    float  time;
};

Texture2D WaterNormalMap1  : register(t0);
Texture2D WaterNormalMap2  : register(t1);
Texture2D HDRMap           : register(t2);
Texture2D DepthMap         : register(t3);
Texture2D NormalMap        : register(t4);
TextureCube EnvironmentMap : register(t5);
Texture2D WaterFoamMap     : register(t6);
Texture2D WaterNoiseMap    : register(t7);

SamplerState LinearWrapSampler  : register(s0);
SamplerState PointSampler       : register(s1);
SamplerState LinearClampSampler : register(s2);
SamplerState PointWrapSampler   : register(s3);

static const uint numWaves = 2;

HullInput WaterVertexShader(VertexInput input)
{
    HullInput output;
    output.position = input.position;
	output.texCoord0 = input.texCoord0;
    
    return output;
}

TessellationPatch WaterTessellation(InputPatch<HullInput, 3> inputPatch, uint patchId : SV_PrimitiveID)
{
    TessellationPatch output;
	output.edges[0] = output.edges[1] = output.edges[2] = tessellationFactor;
    output.inside = tessellationFactor;
               
    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[maxtessfactor(15.0)]
[patchconstantfunc("WaterTessellation")]
DomainInput WaterHullShader(InputPatch<HullInput, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    DomainInput output;
    output.position = patch[pointId].position;
    output.texCoord0 = patch[pointId].texCoord0;

    return output;
}

struct WaveResult
{
    float3 position;
    float3 normal;
    float3 binormal;
    float3 tangent;
};

struct Wave
{
    float3 direction;
    float steepness;
    float waveLength;
    float amplitude;
    float speed;
};

WaveResult CalculateWave(Wave wave, float3 wavePosition, float edgeDampen)
{
    WaveResult result;
    
    float frequency = 2.0 / wave.waveLength;
    float phaseConstant = wave.speed * frequency;
    float qi = wave.steepness / (wave.amplitude * frequency * numWaves);
    float rad = frequency * dot(wave.direction.xz, wavePosition.xz) + time * phaseConstant;
    float sinR = sin(rad);
    float cosR = cos(rad);
    
    result.position.x = wavePosition.x + qi * wave.amplitude * wave.direction.x * cosR * edgeDampen;
    result.position.z = wavePosition.z + qi * wave.amplitude * wave.direction.z * cosR * edgeDampen;
    result.position.y = wave.amplitude * sinR * edgeDampen;
    
    float waFactor = frequency * wave.amplitude;
    float radN = frequency * dot(wave.direction, result.position) + time * phaseConstant;
    float sinN = sin(radN);
    float cosN = cos(radN);
    
    result.binormal.x = 1 - (qi * wave.direction.x * wave.direction.x * waFactor * sinN);
    result.binormal.z = -1 * (qi * wave.direction.x * wave.direction.z * waFactor * sinN);
    result.binormal.y = wave.direction.x * waFactor * cosN;
    
    result.tangent.x = -1 * (qi * wave.direction.x * wave.direction.z * waFactor * sinN);
    result.tangent.z = 1 - (qi * wave.direction.z * wave.direction.z * waFactor * sinN);
    result.tangent.y = wave.direction.z * waFactor * cosN;
    
    result.normal.x = -1 * (wave.direction.x * waFactor * cosN);
    result.normal.z = -1 * (wave.direction.z * waFactor * cosN);
    result.normal.y = 1 - (qi * waFactor * sinN);
    
    result.binormal = normalize(result.binormal);
    result.tangent = normalize(result.tangent);
    result.normal = normalize(result.normal);
    
    return result;
}


[domain("tri")]
PixelInput WaterDomainShader(TessellationPatch input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<DomainInput, 3> patch)
{
    PixelInput output;
    output.position = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
    output.texCoord0 = uvwCoord.x * patch[0].texCoord0 + uvwCoord.y * patch[1].texCoord0 + uvwCoord.z * patch[2].texCoord0;
    
    Wave waves[numWaves];
    waves[0].direction = float3(0.3, 0, -0.7);
    waves[0].steepness = 1.79;
    waves[0].waveLength = 3.75;
    waves[0].amplitude = 0.85;
    waves[0].speed = 12.1;
    
    waves[1].direction = float3(0.5, 0, -0.2);
    waves[1].steepness = 1.79;
    waves[1].waveLength = 4.1;
    waves[1].amplitude = 0.52;
    waves[1].speed = 10.3;
    
    float dampening = 1.0 - pow(saturate(abs(output.texCoord0.z - 0.5) / 0.5), 5.0);
    dampening *= 1.0 - pow(saturate(abs(output.texCoord0.w - 0.5) / 0.5), 5.0);
    
    WaveResult finalWaveResult;
    finalWaveResult.position = float3(0,0,0);
    finalWaveResult.normal = float3(0,0,0);
    finalWaveResult.tangent = float3(0,0,0);
    finalWaveResult.binormal = float3(0,0,0);
    
    for(uint waveId = 0; waveId < numWaves; waveId++)
    {
        WaveResult waveResult = CalculateWave(waves[waveId], output.position.xyz, dampening);
        finalWaveResult.position += waveResult.position;
        finalWaveResult.normal += waveResult.normal;
        finalWaveResult.tangent += waveResult.tangent;
        finalWaveResult.binormal += waveResult.binormal;
    }
    
    finalWaveResult.position -= output.position.xyz * (numWaves - 1);
    finalWaveResult.normal = normalize(finalWaveResult.normal);
    finalWaveResult.tangent = normalize(finalWaveResult.tangent);
    finalWaveResult.binormal = normalize(finalWaveResult.binormal);
    
    output.worldNormalAndHeight.w = finalWaveResult.position.y - output.position.y;
    
    output.position = float4(finalWaveResult.position, 1.0);
    output.positionWorld = mul(output.position, modelMatrix);
    output.positionView = mul(output.positionWorld, viewMatrix);
    output.position = mul(output.positionView, projectionMatrix);
    output.screenPosition = output.position;
    
    output.normal = normalize(mul(finalWaveResult.normal, (float3x3)modelMatrix));
    
    output.worldNormalAndHeight.xyz = finalWaveResult.normal;
    
	output.normal = normalize(mul(float4(output.normal, 0.0), viewMatrix).xyz);
    output.tangent = normalize(mul(finalWaveResult.tangent, (float3x3)modelMatrix));
	output.tangent = normalize(mul(float4(output.tangent, 0.0), viewMatrix).xyz);
    output.binormal = normalize(mul(finalWaveResult.binormal, (float3x3)modelMatrix));
	output.binormal = normalize(mul(float4(output.binormal, 0.0), viewMatrix).xyz);
    
    return output;
}

float4 WaterPixelShader(PixelInput input) : SV_TARGET
{
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    input.binormal = normalize(input.binormal);
    
    float2 normalMapCoords1 = input.texCoord0.xy + time * normalMapScroll.xy * normalMapScrollSpeed.x;
    float2 normalMapCoords2 = input.texCoord0.xy + time * normalMapScroll.zw * normalMapScrollSpeed.y;
    float2 hdrCoords = ((float2(input.screenPosition.x, -input.screenPosition.y) / input.screenPosition.w) * 0.5) + 0.5;
    
    float4 normalMap = (WaterNormalMap1.Sample(LinearWrapSampler, normalMapCoords1) * 2.0) - 1.0;
    float4 normalMap2 = (WaterNormalMap2.Sample(LinearWrapSampler, normalMapCoords2) * 2.0) - 1.0;
    float3x3 texSpace = float3x3(input.tangent, input.binormal, input.normal);
	float3 finalNormal = normalize(mul(normalMap.xyz, texSpace));
    finalNormal += normalize(mul(normalMap2.xyz, texSpace));
    finalNormal = normalize(finalNormal);
    
    float2 distortedTexCoord = hdrCoords + finalNormal.xz * refractionDistortionFactor;
    float distortedDepth = DepthMap.Sample(PointSampler, distortedTexCoord).r;
    float3 distortedPosition = GetWorldPositionFromDepth(distortedTexCoord, distortedDepth, viewProjInvMatrix);
    
    float3 waterColorFactor;
    if(distortedPosition.y < input.positionWorld.y)
    {
        waterColorFactor = waterRefractionColor.rgb * HDRMap.Sample(PointSampler, distortedTexCoord).rgb;
    }
    else
    {
        waterColorFactor = waterRefractionColor.rgb * HDRMap.Sample(PointSampler, hdrCoords).rgb; 
    }
    
    float linearRoughness = roughness * roughness;
    float3 viewDir = -normalize(input.positionView.xyz);
    float3 lightDir = -lightDirection.xyz;
    float3 half = normalize(viewDir + lightDir);
    float nDotL = saturate(dot(finalNormal, lightDir));
    float nDotV = abs(dot(finalNormal, viewDir)) + EPSILON;
    float nDotH = saturate(dot(finalNormal, half));
    float lDotH = saturate(dot(lightDir, half));
    
    float3 f0 = 0.16 * reflectance * reflectance;
    float normalDistribution = CalculateNormalDistributionGGX(linearRoughness, nDotH);
    float3 fresnelReflectance = CalculateSchlickFresnelReflectance(lDotH, f0);
    float geometryTerm = CalculateSmithGGXGeometryTerm(linearRoughness, nDotL, nDotV);
    
    float3 specularFactor = (geometryTerm * normalDistribution) * fresnelReflectance * specIntensity * nDotL;

    float maxStep = 20.0;
    float maxStepBack = 10.0;
    float steps = 0;
    float3 currentPos = input.positionView.xyz;
    float4 texPos = float4(0,0,0,0);
    float sceneZ = 0;
    float actualStep = ssrSettings.y;
    
    float3 reflection = normalize(reflect(-viewDir, finalNormal));
    
    while(steps < ssrSettings.y)
    {	
        currentPos += reflection.xyz * ssrSettings.x;
        texPos = mul(float4(-currentPos, 1), projectionMatrix);
        
        if(abs(texPos.w) < EPSILON)
        {
            texPos.w = EPSILON;
        }
        
        texPos.xy /= texPos.w;
        texPos.xy = float2(texPos.x, -texPos.y) * 0.5 + 0.5; 

        sceneZ = DepthMap.SampleLevel(PointSampler, texPos.xy, 0).r;
        sceneZ = GetViewPositionFromDepth(texPos.xy, sceneZ, projectionInverseMatrix).z;
        
        if (sceneZ <= currentPos.z)
        {
            actualStep = steps;
            steps = ssrSettings.y;				
        }
        else
        {
            steps++;
        }
    }
    
    if (actualStep < ssrSettings.y)
    {
        steps = 0;		
        while(steps < ssrSettings.z)
        {	
            currentPos -= reflection.xyz * ssrSettings.x / ssrSettings.z;
            texPos = mul(float4(-currentPos, 1), projectionMatrix);
            
            if(abs(texPos.w) < EPSILON)
            {
                texPos.w = EPSILON;
            }
            
            texPos.xy /= texPos.w;
            texPos.xy = float2(texPos.x, -texPos.y) * 0.5 + 0.5; 
    
            sceneZ = DepthMap.SampleLevel(PointSampler, texPos.xy, 0).r;
            sceneZ = GetViewPositionFromDepth(texPos.xy, sceneZ, projectionInverseMatrix).z;
            
            if(sceneZ > currentPos.z)
            {
                steps = ssrSettings.z;
            }					
            else
            {
                steps++;
            }
        }
    }
    
    float3 reflectionNormal = DecodeSphereMap(NormalMap.Sample(PointSampler, texPos.xy).xy);
    float2 distanceFactor = float2(distance(0.5, hdrCoords.x), distance(0.5, hdrCoords.y)) * 2;
    
    float ssrFactor = 1.0 * pow( 1.0 - abs(nDotV), 1.0)
					  * (1.0 - actualStep / ssrSettings.y)
					  * saturate(1.0 - distanceFactor.x - distanceFactor.y)
					  * (1.0 / (1.0 + abs(sceneZ - currentPos.z) * ssrSettings.w))
					  * (1.0 - saturate(dot(reflectionNormal, finalNormal)));
    
    float3 reflectionColor = HDRMap.Sample(PointSampler, texPos.xy).rgb * ssrFactor;
    
	float3 envReflection = mul(reflection, (float3x3)viewInverseMatrix);
    float3 skyboxColor = EnvironmentMap.Sample(LinearClampSampler, envReflection).rgb * waterSurfaceColor.rgb;
    
    float3 finalReflectionColor = lerp(skyboxColor, reflectionColor, ssrFactor);
    
    float sceneDepth = DepthMap.Sample(PointSampler, hdrCoords).r;
    float3 scenePosition = GetWorldPositionFromDepth(hdrCoords, sceneDepth, viewProjInvMatrix);
    float depthSoftenedAlpha = saturate(distance(scenePosition, input.positionWorld.xyz) / 0.5);
    
    float3 waterColorDepthPosition = lerp(scenePosition, distortedPosition, distortedPosition.y < input.positionWorld.y ? 1.0 : 0);
    waterColorFactor = lerp(waterColorFactor, waterRefractionColor.rgb, saturate((input.positionWorld.y - waterColorDepthPosition.y) / 2.5));
    
    float waveTopReflection = pow(1.0 - saturate(dot(input.normal, viewDir)), 3);
    float3 waterBaseColor = lerp(waterColorFactor, finalReflectionColor, saturate(saturate(length(input.positionView.xyz) / 15.0) + waveTopReflection));
    
    float specularNoise = WaterNoiseMap.Sample(LinearWrapSampler, normalMapCoords1 * 0.5).r;
    specularNoise *= WaterNoiseMap.Sample(LinearWrapSampler, normalMapCoords2 * 0.5).r;
    specularNoise *= WaterNoiseMap.Sample(LinearWrapSampler, input.texCoord0.xy * 0.5).r;
    
    float3 finalWaterColor = waterBaseColor + specularFactor * specularNoise;
    
    float3 foamColor = WaterFoamMap.Sample(LinearWrapSampler, (normalMapCoords1 + normalMapCoords2) / 0.5).rgb;
    float foamNoise = WaterNoiseMap.Sample(LinearWrapSampler, input.texCoord0.xy * 2).r;
    float foamAmount = saturate((input.worldNormalAndHeight.w - 0.8) / 0.4) * pow(saturate(dot(input.worldNormalAndHeight.xyz, float3(0, 1, 0))), 80) * foamNoise;
    foamAmount += pow((1.0 - depthSoftenedAlpha), 3);
    finalWaterColor = lerp(finalWaterColor, foamColor * 4, saturate(foamAmount) * depthSoftenedAlpha);
    
    return float4(finalWaterColor, depthSoftenedAlpha);
}