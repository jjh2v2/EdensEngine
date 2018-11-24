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
    float3 lightTexCoord;        
    float3 lightTexCoordDX;
    float3 lightTexCoordDY;
    float3 positionView;
	float  zDepth;
};

cbuffer LightingPassBuffer : register(b0)
{
	matrix pfViewMatrix;
    matrix pfProjectionMatrix;
	matrix pfViewToLightProjMatrix;
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
Texture2DArray ShadowTextures[4] : register(t4);
TextureCube EnvironmentMap : register(t8);
Texture2D<float2> EnvBRDFLookupTexture : register(t9);
StructuredBuffer<ShadowPartition> ShadowPartitions : register(t10);
Texture2D<float> ShadowTexture : register(t11);
SamplerState ShadowSampler : register(s0);
SamplerState EnvironmentSampler : register(s1);
SamplerState ShadowLinearSampler : register(s2);

LightingSurface GetLightingSurfaceFromGBuffer(uint2 coords)
{
    float2 screenPixelOffset = float2(2.0f, -2.0f) / pfBufferDimensions;
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    
    LightingSurface surface;
	float4 normal    = GBufferTextures[1][coords];
	surface.albedo   = GBufferTextures[0][coords];
	surface.material = GBufferTextures[2][coords];
	surface.zDepth   = GBufferTextures[3][coords].r;
    surface.normal   = DecodeSphereMap(normal.xy);
    surface.positionView = GetViewPosition(coords, pfBufferDimensions, surface.zDepth, pfProjectionMatrix);
    
	float2 positionScreenX = positionScreen + float2(screenPixelOffset.x, 0.0f);
    float2 positionScreenY = positionScreen + float2(0.0f, screenPixelOffset.y);
	float3 positionViewDX = GetViewPosition(positionScreenX, surface.positionView.z + normal.z, pfProjectionMatrix) - surface.positionView;
    float3 positionViewDY = GetViewPosition(positionScreenY, surface.positionView.z + normal.w, pfProjectionMatrix) - surface.positionView;
    surface.lightTexCoord = ViewPositionToLightTexCoord(surface.positionView, pfViewToLightProjMatrix);
    surface.lightTexCoordDX = 0.5 * (ViewPositionToLightTexCoord(surface.positionView + 2.0 * positionViewDX, pfViewToLightProjMatrix) - surface.lightTexCoord);
    surface.lightTexCoordDY = 0.5 * (ViewPositionToLightTexCoord(surface.positionView + 2.0 * positionViewDY, pfViewToLightProjMatrix) - surface.lightTexCoord);
    
    return surface;
}