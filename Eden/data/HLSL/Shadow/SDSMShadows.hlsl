#include "../Common/ShaderCommon.hlsl"
#include "../Common/ShadowShaderCommon.hlsl"

Texture2D DepthBufferTexture : register(t0);

RWStructuredBuffer<ShadowPartition> ShadowPartitionsRW : register(u0);
RWStructuredBuffer<ShadowPartitionUint> ShadowPartitionsRWUint : register(u0);
StructuredBuffer<ShadowPartition> ShadowPartitionsR : register(t1);

RWStructuredBuffer<ShadowPartitionBoundUint> ShadowPartitionBoundsRWUint : register(u1);
StructuredBuffer<ShadowPartitionBoundFloat> ShadowPartitionBoundsR : register(t2);

#define NUM_SHADOW_PARTITIONS 4
#define DEPTH_BOUNDS_THREADS_PER_DIMENSION 8
#define DEPTH_BOUNDS_BLOCK_SIZE (DEPTH_BOUNDS_THREADS_PER_DIMENSION*DEPTH_BOUNDS_THREADS_PER_DIMENSION)

#define PARTITION_BOUNDS_THREADS_PER_DIMENSION 8
#define PARTITION_BOUNDS_BLOCK_SIZE (PARTITION_BOUNDS_THREADS_PER_DIMENSION*PARTITION_BOUNDS_THREADS_PER_DIMENSION)
#define PARTITION_BOUNDS_SHARED_MEMORY_SIZE (NUM_SHADOW_PARTITIONS * PARTITION_BOUNDS_BLOCK_SIZE)

groupshared float SharedMinDepth[DEPTH_BOUNDS_BLOCK_SIZE];
groupshared float SharedMaxDepth[DEPTH_BOUNDS_BLOCK_SIZE];
groupshared float3 PartitionBoundsMin[PARTITION_BOUNDS_SHARED_MEMORY_SIZE];
groupshared float3 PartitionBoundsMax[PARTITION_BOUNDS_SHARED_MEMORY_SIZE];

cbuffer SDSMBuffer: register(b0)
{
    matrix pfCameraProj;
    matrix pfCameraViewToLightProj;
    float4 pfLightSpaceBorder;
    float4 pfMaxScale;
	float2 pfBufferDimensions;
	float2 pfCameraNearFar;
    float  pfDilationFactor;
    uint   pfReduceTileDim;
};

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void ClearShadowPartitions(uint groupIndex : SV_GroupIndex)
{
    ShadowPartitionsRWUint[groupIndex].intervalBegin = 0x7F7FFFFF; //float max
    ShadowPartitionsRWUint[groupIndex].intervalEnd = 0;
}

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void ClearShadowPartitionBounds(uint groupIndex : SV_GroupIndex)
{
    ShadowPartitionBoundUint bound;
    bound.minCoord = uint3(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF); //float max
    bound.maxCoord = uint3(0,0,0);
    bound.padding = 0;
    bound.padding2 = 0;
    
    ShadowPartitionBoundsRWUint[groupIndex] = bound;
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
            float viewPositionZ = GetViewPositionZ(texCoords, pfBufferDimensions, zDepth, pfCameraProj);
            
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
        InterlockedMin(ShadowPartitionsRWUint[0].intervalBegin, asuint(SharedMinDepth[0]));
        InterlockedMax(ShadowPartitionsRWUint[NUM_SHADOW_PARTITIONS - 1].intervalEnd, asuint(SharedMaxDepth[0]));
    }
}

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void CalculateLogPartitionsFromDepthBounds(uint groupIndex : SV_GroupIndex)
{
    float minZ = ShadowPartitionsRW[0].intervalBegin;
    float maxZ = ShadowPartitionsRW[NUM_SHADOW_PARTITIONS - 1].intervalEnd;

    ShadowPartitionsRW[groupIndex].intervalBegin = groupIndex == 0 ? pfCameraNearFar.x : GetLogPartitionFromDepthRange(groupIndex, NUM_SHADOW_PARTITIONS, minZ, maxZ);
    ShadowPartitionsRW[groupIndex].intervalEnd = groupIndex == (NUM_SHADOW_PARTITIONS - 1) ? pfCameraNearFar.y : GetLogPartitionFromDepthRange(groupIndex + 1, NUM_SHADOW_PARTITIONS, minZ, maxZ);
}

[numthreads(PARTITION_BOUNDS_THREADS_PER_DIMENSION, PARTITION_BOUNDS_THREADS_PER_DIMENSION, 1)]
void CalculatePartitionBounds(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
    ShadowPartitionBoundFloat partitionBounds[NUM_SHADOW_PARTITIONS];
    
    uint3 minBound = uint3(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF); //float max
    ShadowPartitionBoundFloat emptyBounds;
    emptyBounds.minCoord = asfloat(minBound);
    emptyBounds.maxCoord = float3(0,0,0);
    emptyBounds.padding = 0;
    emptyBounds.padding2 = 0;
    
    [unroll] 
    for (uint partitionIndex = 0; partitionIndex < NUM_SHADOW_PARTITIONS; partitionIndex++) 
    {
        partitionBounds[partitionIndex] = emptyBounds;
    }
    
    float nearZ = ShadowPartitionsR[0].intervalBegin;
    float farZ = ShadowPartitionsR[NUM_SHADOW_PARTITIONS - 1].intervalEnd;
    uint2 tileStart = groupId.xy * pfReduceTileDim.xx + groupThreadId.xy;
    
    for (uint tileY = 0; tileY < pfReduceTileDim; tileY += PARTITION_BOUNDS_THREADS_PER_DIMENSION) 
    {
        for (uint tileX = 0; tileX < pfReduceTileDim; tileX += PARTITION_BOUNDS_THREADS_PER_DIMENSION) 
        {
            uint2 texCoords = tileStart + uint2(tileX, tileY);
            float zDepth = DepthBufferTexture[texCoords].r;
            float3 viewPosition = GetViewPosition(texCoords, pfBufferDimensions, zDepth, pfCameraProj);
            float3 lightCoord = ViewPositionToLightTexCoord(viewPosition, pfCameraViewToLightProj);
            
            if (viewPosition.z >= nearZ && viewPosition.z < farZ) 
            {
                uint partition = 0;
                [unroll] 
                for (uint i = 0; i < (NUM_SHADOW_PARTITIONS - 1); i++) 
                {
                    [flatten] 
                    if (viewPosition.z >= ShadowPartitionsR[i].intervalEnd) 
                    {
                        partition++;
                    }
                }

                partitionBounds[partition].minCoord = min(partitionBounds[partition].minCoord, lightCoord.xyz);
                partitionBounds[partition].maxCoord = max(partitionBounds[partition].maxCoord, lightCoord.xyz);
            }
        }
    }
    
    [unroll] 
    for (uint p = 0; p < NUM_SHADOW_PARTITIONS; p++) 
    {
        uint index = (groupIndex * NUM_SHADOW_PARTITIONS + p);
        PartitionBoundsMin[index] = partitionBounds[p].minCoord;
        PartitionBoundsMax[index] = partitionBounds[p].maxCoord;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint offset = (PARTITION_BOUNDS_SHARED_MEMORY_SIZE >> 1); offset >= NUM_SHADOW_PARTITIONS; offset >>= 1) 
    {
        for (uint boundIndex = groupIndex; boundIndex < offset; boundIndex += PARTITION_BOUNDS_BLOCK_SIZE) 
        {
            PartitionBoundsMin[boundIndex] = min(PartitionBoundsMin[boundIndex], PartitionBoundsMin[offset + boundIndex]);
            PartitionBoundsMax[boundIndex] = max(PartitionBoundsMax[boundIndex], PartitionBoundsMax[offset + boundIndex]);
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupIndex < NUM_SHADOW_PARTITIONS) 
    {
        InterlockedMin(ShadowPartitionBoundsRWUint[groupIndex].minCoord.x, asuint(PartitionBoundsMin[groupIndex].x));
        InterlockedMin(ShadowPartitionBoundsRWUint[groupIndex].minCoord.y, asuint(PartitionBoundsMin[groupIndex].y));
        InterlockedMin(ShadowPartitionBoundsRWUint[groupIndex].minCoord.z, asuint(PartitionBoundsMin[groupIndex].z));
        InterlockedMax(ShadowPartitionBoundsRWUint[groupIndex].maxCoord.x, asuint(PartitionBoundsMax[groupIndex].x));
        InterlockedMax(ShadowPartitionBoundsRWUint[groupIndex].maxCoord.y, asuint(PartitionBoundsMax[groupIndex].y));
        InterlockedMax(ShadowPartitionBoundsRWUint[groupIndex].maxCoord.z, asuint(PartitionBoundsMax[groupIndex].z));
    }
}

[numthreads(NUM_SHADOW_PARTITIONS, 1, 1)]
void FinalizePartitions(uint groupIndex : SV_GroupIndex)
{
    float3 scale = float3(0,0,0);
    float3 bias = float3(0,0,0);
    
    ComputePartitionScaleAndBias(ShadowPartitionBoundsR[groupIndex], pfLightSpaceBorder, pfMaxScale, pfDilationFactor, scale, bias);
    
    ShadowPartitionsRW[groupIndex].scale = scale;
    ShadowPartitionsRW[groupIndex].bias = bias;
}