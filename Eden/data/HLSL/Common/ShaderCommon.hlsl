struct VertexInput
{
    float4 position  : POSITION;
    float4 texCoord0 : TEXCOORD0;
	float3 normal    : NORMAL;
	float3 tangent   : TANGENT;
    float3 binormal  : BINORMAL;
	float4 color     : COLOR;
};

struct GBufferVertexOutput
{
    float4 position  	: SV_POSITION;
    float2 texCoord0 	: TEXCOORD0;
	float3 normal    	: NORMAL;
	float3 tangent   	: TANGENT;
    float3 binormal  	: BINORMAL;
	float4 positionView : POSITIONVIEW;
};

struct GBufferPixelOutput
{
	float4 albedo   : SV_Target0;
    float4 normals  : SV_Target1;
	float4 material : SV_Target2;
};

cbuffer PerFrameBuffer(b0)
{
	matrix pfViewMatrix;
    matrix pfProjectionMatrix;
};

cbuffer PerObjectBuffer(b1)
{
	matrix 	poWorldMatrix;
    float4 	poDiffuseColor;
	float2 	poTile;
	float 	poRoughness;
	float 	poMetalness;
	float 	poMaterialIntensity;
};

Texture2D DiffuseTexture(t0);
SamplerState DiffuseSampler(s0);

#ifdef USE_NORMAL_MAP
	Texture2D NormalMap(t1);
	SamplerState NormalMapSampler(s1);
#endif

#ifdef USE_ROUGHMETAL_MAP
	Texture2D RoughMetalMap(t2);
	SamplerState RoughMetalMapSampler(s2);
#endif

#ifdef USE_DEPTH_MAP
	Texture2D DepthMap(t3);
	SamplerState DepthMapSampler(s3);
#endif
