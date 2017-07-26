#include "../Common/ShaderCommon.hlsl"

struct LightingMainVertexOutput
{
    float4 position  : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

struct LightingSurface
{
    float4 albedo;
	float3 normal;
	float4 material;
    float3 lightTexCoord;        
    float3 lightTexCoordDX;
    float3 lightTexCoordDY;
	float  zDepth;
};

cbuffer LightingPassPerFrameBuffer : register(b0)
{
	matrix pfViewMatrix;
    matrix pfProjectionMatrix;
	matrix pfViewToLightProjMatrix;
	matrix pfViewInvMatrix;
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

Texture2D GBufferTextures[4];
Texture2DArray ShadowTextures[4];
TextureCube EnvironmentMap;
Texture2D<float2> EnvBRDFLookupTexture;
StructuredBuffer<ShadowPartition> ShadowPartitions;
SamplerState ShadowSampler;
SamplerState EnvironmentSampler;

LightingSurface GetLightingSurfaceFromGBuffer(uint2 coords, float3 positionView)
{
    float2 bufferDimensions;
    GBufferTextures[0].GetDimensions(bufferDimensions.x, bufferDimensions.y);
    
    float2 screenPixelOffset = float2(2.0f, -2.0f) / bufferDimensions;
    float2 positionScreen = (float2(coords.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    
    LightingSurface surface;
	float4 normal    = GBufferTextures[1][coords];
	surface.albedo   = GBufferTextures[0][coords];
	surface.material = GBufferTextures[2][coords];
	surface.zDepth   = GBufferTextures[3][coords].r;
    surface.normal   = DecodeSphereMap(normal.xy);
    
	float2 positionScreenX = positionScreen + float2(screenPixelOffset.x, 0.0f);
    float2 positionScreenY = positionScreen + float2(0.0f, screenPixelOffset.y);
	float3 positionViewDX = GetViewPosition(positionScreenX, positionView.z + normal.z) - positionView;
    float3 positionViewDY = GetViewPosition(positionScreenY, positionView.z + normal.w) - positionView;
    surface.lightTexCoord = ViewPositionToLightTexCoord(positionView);
    surface.lightTexCoordDX = 0.5 * (ViewPositionToLightTexCoord(positionView + 2.0 * positionViewDX) - surface.lightTexCoord);
    surface.lightTexCoordDY = 0.5 * (ViewPositionToLightTexCoord(positionView + 2.0 * positionViewDY) - surface.lightTexCoord);
    
    return surface;
}