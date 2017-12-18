#include "../Common/ShaderCommon.hlsl"
#include "../Common/ShadowShaderCommon.hlsl"

RWStructuredBuffer<ShadowPartition> ShadowPartitions : register(u0);
RWStructuredBuffer<ShadowPartition> ShadowPartitionsUint : register(u0);
Texture2D DepthBufferTexture : register(t0);

#define NUM_SHADOW_PARTITIONS 4
#define DEPTH_BOUNDS_THREADS_PER_DIMENSION 8
#define DEPTH_BOUNDS_BLOCK_SIZE (DEPTH_BOUNDS_THREADS_PER_DIMENSION*DEPTH_BOUNDS_THREADS_PER_DIMENSION)

groupshared float SharedMinDepth[DEPTH_BOUNDS_BLOCK_SIZE];
groupshared float SharedMaxDepth[DEPTH_BOUNDS_BLOCK_SIZE];

cbuffer SDSMBuffer: register(b0)
{
	matrix pfCameraWorldViewProj;
    matrix pfCameraWorldView;
    matrix pfCameraViewProj;
    matrix pfCameraProj;
    matrix pfLightWorldViewProj;
    matrix pfCameraViewToLightProj;
	matrix pfWorldMatrix;
	matrix pfViewMatrix;
	matrix pfProjMatrix;
    float4 pfLightDirection;
    float4 pfBlurSizeLightSpace;
    float4 pfLightSpaceBorder;
    float4 pfMaxScale;
	float2 pfBufferDimensions;
	float2 pfCameraNearFar;
    float  pfDilationFactor;
    uint   pfScatterTileDim;
    uint   pfReduceTileDim;
};

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void ClearShadowPartitions(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    ShadowPartitionsUint[dispatchThreadID.x].intervalBegin = 0x7F7FFFFF; //float max
    ShadowPartitionsUint[dispatchThreadID.x].intervalEnd = 0;
}

[numthreads(DEPTH_BOUNDS_THREADS_PER_DIMENSION, DEPTH_BOUNDS_THREADS_PER_DIMENSION, 1)]
void CalculateDepthBufferBounds(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    float minDepth = pfCameraNearFar.y;
    float maxDepth = pfCameraNearFar.x;
    uint2 tileStart = groupId.xy * pfReduceTileDim.xx + groupThreadId.xy;
    
    for (uint tileY = 0; tileY < pfReduceTileDim; tileY += DEPTH_BOUNDS_THREADS_PER_DIMENSION) 
    {
        for (uint tileX = 0; tileX < pfReduceTileDim; tileX += DEPTH_BOUNDS_THREADS_PER_DIMENSION) 
        {
            uint2 texCoords = tileStart + uint2(tileX, tileY);
            float zDepth = DepthBufferTexture[texCoords].r;
            float viewPositionZ = GetViewPosition(texCoords, pfBufferDimensions, zDepth, pfCameraProj).z;
            if (viewPositionZ >= pfCameraNearFar.x && viewPositionZ < pfCameraNearFar.y) 
            {
                minDepth = min(minDepth, viewPositionZ);
                maxDepth = max(maxDepth, viewPositionZ);
            }
        }
    }
    
    SharedMinDepth[groupIndex] = minDepth;
    SharedMaxDepth[groupIndex] = maxDepth;
    GroupMemoryBarrierWithGroupSync();

    for (uint offset = (DEPTH_BOUNDS_BLOCK_SIZE >> 1); offset > 0; offset >>= 1) 
    {
        if (groupIndex < offset) 
        {
            SharedMinDepth[groupIndex] = min(SharedMinDepth[groupIndex], SharedMinDepth[offset + groupIndex]);
            SharedMaxDepth[groupIndex] = max(SharedMaxDepth[groupIndex], SharedMaxDepth[offset + groupIndex]);
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupIndex == 0) 
    {
        InterlockedMin(ShadowPartitionsUint[0].intervalBegin, asuint(SharedMinDepth[0]));
        InterlockedMax(ShadowPartitionsUint[NUM_SHADOW_PARTITIONS - 1].intervalEnd, asuint(SharedMaxDepth[0]));
    }
}