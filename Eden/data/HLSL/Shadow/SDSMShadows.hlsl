#include "../Common/ShaderCommon.hlsl"
#include "../Common/ShadowShaderCommon.hlsl"

Texture2D DepthBufferTexture : register(t0);

RWStructuredBuffer<ShadowPartition> ShadowPartitions : register(u0);
RWStructuredBuffer<ShadowPartitionUint> ShadowPartitionsUint : register(u0);

RWStructuredBuffer<ShadowPartitionBoundUint> ShadowPartitionBoundsUint : register(u1);
StructuredBuffer<ShadowPartitionBoundFloat> ShadowPartitionBounds : register(t1);

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

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void ClearShadowPartitionBounds(uint groupIndex : SV_GroupIndex)
{
    ShadowPartitionBoundUint bound;
    bound.minCoord = uint3(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF); //float max
    bound.maxCoord = uint3(0,0,0);
    
    ShadowPartitionBoundsUint[groupIndex] = bound;
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

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void CalculateLogPartitionsFromDepthBounds(uint groupIndex : SV_GroupIndex)
{
    float minZ = ShadowPartitions[0].intervalBegin;
    float maxZ = ShadowPartitions[NUM_SHADOW_PARTITIONS - 1].intervalEnd;

    ShadowPartitions[groupIndex].intervalBegin = groupIndex == 0 ? pfCameraNearFar.x : GetLogPartitionFromDepthRange(groupIndex, NUM_SHADOW_PARTITIONS, minZ, maxZ);
    ShadowPartitions[groupIndex].intervalEnd = groupIndex == (NUM_SHADOW_PARTITIONS - 1) ? pfCameraNearFar.y : GetLogPartitionFromDepthRange(groupIndex + 1, NUM_SHADOW_PARTITIONS, minZ, maxZ);
}


[numthreads(REDUCE_BOUNDS_BLOCK_X, REDUCE_BOUNDS_BLOCK_Y, 1)]
void ReduceBoundsFromGBuffer(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    // Initialize stack copy of partition data for this thread
    BoundsFloat boundsReduce[NUM_SHADOW_PARTITIONS];
    {
        [unroll] for (uint partition = 0; partition < NUM_SHADOW_PARTITIONS; ++partition) 
        {
            boundsReduce[partition] = EmptyBoundsFloat();
        }
    }
    
    // Loop over tile and reduce into local memory
    float nearZ = gPartitionsReadOnly[0].intervalBegin;
    float farZ = gPartitionsReadOnly[NUM_SHADOW_PARTITIONS - 1].intervalEnd;
    {
        uint2 tileStart = groupId.xy * mReduceTileDim.xx + groupThreadId.xy;
        for (uint tileY = 0; tileY < mReduceTileDim; tileY += REDUCE_BOUNDS_BLOCK_Y) 
        {
            for (uint tileX = 0; tileX < mReduceTileDim; tileX += REDUCE_BOUNDS_BLOCK_X) 
            {
                uint2 globalCoords = tileStart + uint2(tileX, tileY);
                SurfaceData data = ComputeSurfaceData(globalCoords);
                
                if (data.positionView.z >= nearZ && data.positionView.z < farZ) 
                {
                    uint partition = 0;
                    [unroll] for (uint i = 0; i < (NUM_SHADOW_PARTITIONS - 1); ++i) 
                    {
                        [flatten] if (data.positionView.z >= gPartitionsReadOnly[i].intervalEnd) 
                        {
                            ++partition;
                        }
                    }

                    boundsReduce[partition].minCoord = min(boundsReduce[partition].minCoord, data.lightTexCoord.xyz);
                    boundsReduce[partition].maxCoord = max(boundsReduce[partition].maxCoord, data.lightTexCoord.xyz);
                }
            }
        }
    }
    
    
    // Copy result to shared memory for reduction
    {
        [unroll] for (uint partition = 0; partition < NUM_SHADOW_PARTITIONS; ++partition) 
        {
            uint index = (groupIndex * NUM_SHADOW_PARTITIONS + partition);
            sBoundsMin[index] = boundsReduce[partition].minCoord;
            sBoundsMax[index] = boundsReduce[partition].maxCoord;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint offset = (REDUCE_BOUNDS_SHARED_MEMORY_ARRAY_SIZE >> 1); offset >= NUM_SHADOW_PARTITIONS; offset >>= 1) 
    {
        for (uint i = groupIndex; i < offset; i += REDUCE_BOUNDS_BLOCK_SIZE) 
        {
            sBoundsMin[i] = min(sBoundsMin[i], sBoundsMin[offset + i]);
            sBoundsMax[i] = max(sBoundsMax[i], sBoundsMax[offset + i]);
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // Now write out the result from this pass
    if (groupIndex < NUM_SHADOW_PARTITIONS) 
    {
        InterlockedMin(gPartitionBoundsUint[groupIndex].minCoord.x, asuint(sBoundsMin[groupIndex].x));
        InterlockedMin(gPartitionBoundsUint[groupIndex].minCoord.y, asuint(sBoundsMin[groupIndex].y));
        InterlockedMin(gPartitionBoundsUint[groupIndex].minCoord.z, asuint(sBoundsMin[groupIndex].z));
        InterlockedMax(gPartitionBoundsUint[groupIndex].maxCoord.x, asuint(sBoundsMax[groupIndex].x));
        InterlockedMax(gPartitionBoundsUint[groupIndex].maxCoord.y, asuint(sBoundsMax[groupIndex].y));
        InterlockedMax(gPartitionBoundsUint[groupIndex].maxCoord.z, asuint(sBoundsMax[groupIndex].z));
    }
}
