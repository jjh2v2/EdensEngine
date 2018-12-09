#include "../Common/ShadowShaderCommon.hlsl"
#include "../Common/ShaderCommon.hlsl"

struct SDSMShadowAccumulationVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

struct ShadowSurface
{
    float3 positionView;
    float3 lightTexCoord;        
    float3 lightTexCoordDX;
    float3 lightTexCoordDY;
};

cbuffer SDSMAccumulationBuffer : register(b0)
{
    matrix pfProjectionMatrix;
	matrix pfViewToLightProjMatrix;
    float2 pfBufferDimensions;
};

Texture2D GBufferTextures[2] : register(t0);
Texture2DArray ShadowTextures[4] : register(t2);
StructuredBuffer<ShadowPartition> ShadowPartitions : register(t6);
SamplerState ShadowSampler : register(s0);

ShadowSurface GetShadowSurface(uint2 coords)
{
    float2 screenPixelOffset = float2(2.0f, -2.0f) / pfBufferDimensions;
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    
    ShadowSurface surface;
	float2 positionViewDDXY = GBufferTextures[0][coords].zw;
	float zDepth = GBufferTextures[1][coords].r;
    surface.positionView = GetViewPosition(coords, pfBufferDimensions, zDepth, pfProjectionMatrix);
    
	float2 positionScreenX = positionScreen + float2(screenPixelOffset.x, 0.0f);
    float2 positionScreenY = positionScreen + float2(0.0f, screenPixelOffset.y);
	float3 positionViewDX = GetViewPosition(positionScreenX, surface.positionView.z + positionViewDDXY.x, pfProjectionMatrix) - surface.positionView;
    float3 positionViewDY = GetViewPosition(positionScreenY, surface.positionView.z + positionViewDDXY.y, pfProjectionMatrix) - surface.positionView;
    surface.lightTexCoord = ViewPositionToLightTexCoord(surface.positionView, pfViewToLightProjMatrix);
    surface.lightTexCoordDX = 0.5 * (ViewPositionToLightTexCoord(surface.positionView + 2.0 * positionViewDX, pfViewToLightProjMatrix) - surface.lightTexCoord);
    surface.lightTexCoordDY = 0.5 * (ViewPositionToLightTexCoord(surface.positionView + 2.0 * positionViewDY, pfViewToLightProjMatrix) - surface.lightTexCoord);
    
    return surface;
}

SDSMShadowAccumulationVertexOutput SDSMShadowAccumulationVertexShader(uint vertexID : SV_VertexID)
{
    SDSMShadowAccumulationVertexOutput output;
    output.texCoord0 = float2((vertexID << 1) & 2, vertexID & 2);
    output.position  = float4(output.texCoord0 * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

float SDSMShadowAccumulationPixelShader(SDSMShadowAccumulationVertexOutput input) : SV_Target
{
    ShadowPartition partition;
    partition.scale = float3(0,0,0); 
    partition.bias = float3(0,0,0);
    partition.intervalBegin = 0;
    partition.intervalEnd = 0;
    
    float3 shadowTexCoord = float3(0,0,0);
	float3 shadowTexCoordDX = float3(0,0,0);
	float3 shadowTexCoordDY = float3(0,0,0);
	float4 shadowOccluder = float4(0,0,0,0);

    ShadowSurface surface = GetShadowSurface(uint2(input.position.xy));
    
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
		return 1.0;
	}
    
    return GetShadowContribution(shadowTexCoord, shadowTexCoordDX, shadowTexCoordDY, saturate(shadowTexCoord.z), partition, shadowOccluder);
}