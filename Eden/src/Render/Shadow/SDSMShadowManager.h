#pragma once
#include "Render/Graphics/GraphicsManager.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

#define SDSM_SHADOW_PARTITION_COUNT 4

class Camera;
class SceneEntity;

class SDSMShadowManager
{
public:
    struct SDSMShadowPreferences
    {
        SDSMShadowPreferences()
        {
            UseSoftShadows = false; //not supported yet
            SofteningAmount = 0.08f;
            MaxSofteningFilter = 8.0f;
            ShadowAntiAliasingSamples = 4;	//also in the evsm shader, change there
            ShadowTextureSize = 1024;
            ShadowTextureMipLevels = 6;
            PartitionCount = SDSM_SHADOW_PARTITION_COUNT;  //only currently handling exactly 4 in the shaders
        }

        bool UseSoftShadows;
        float SofteningAmount;
        float MaxSofteningFilter;
        uint32 ShadowAntiAliasingSamples;
        uint32 ShadowTextureSize;
        uint32 PartitionCount;
        uint32 ShadowTextureMipLevels;
    };

    SDSMShadowManager(GraphicsManager *graphicsManager);
    ~SDSMShadowManager();

    void ComputeShadowPartitions(Camera *camera, D3DXMATRIX &lightViewMatrix, D3DXMATRIX &lightProjectionMatrix, DepthStencilTarget *depthStencil, uint64 gbufferPassFence);
    void RenderShadowMapPartitions(const D3DXMATRIX &lightViewProjMatrix, DynamicArray<SceneEntity*> &shadowEntities);

    RenderTarget *GetShadowEVSMTexture(uint32 index) { return mShadowEVSMTextures[index]; }
    StructuredBuffer *GetShadowPartitionBuffer() { return mShadowPartitionBuffer; }

private:
    void RenderShadowDepth(uint32 partitionIndex, RenderPassContext *renderPassContext, const D3DXMATRIX &lightViewProjMatrix, DynamicArray<SceneEntity*> &shadowEntities);
    void ConvertToEVSM(uint32 partitionIndex);
    void ApplyBlur();
    void GenerateMipsForShadowMap(uint32 partitionIndex, RenderPassContext *renderPassContext);

    GraphicsManager *mGraphicsManager;
    ConstantBuffer *mSDSMBuffers[FRAME_BUFFER_COUNT];
    StructuredBuffer *mShadowPartitionBuffer;
    StructuredBuffer *mShadowPartitionBoundsBuffer;
    DepthStencilTarget *mShadowDepthTarget;
    RenderTarget *mShadowEVSMTextures[SDSM_SHADOW_PARTITION_COUNT];
    RenderTarget *mShadowEVSMBlurTexture;

    DynamicArray<ConstantBuffer*> mPartitionIndexBuffers; //these get set once at construction and never again, so we don't need them per frame buffer

    ShaderPSO *mClearShadowPartitionsShader;
    ShaderPSO *mCalculateDepthBufferBoundsShader;
    ShaderPSO *mCalculateLogPartitionsFromDepthBoundsShader;
    ShaderPSO *mClearShadowPartitionBoundsShader;
    ShaderPSO *mCalculatePartitionBoundsShader;
    ShaderPSO *mFinalizePartitionsShader;

    ShaderPSO *mShadowMapShader;
    ShaderPSO *mShadowMapEVSMShader;
    ShaderPSO *mGenerateMipShader;

    Vector2 mBlurSizeInLightSpace;
    SDSMShadowPreferences mShadowPreferences;
    SDSMBuffer mCurrentSDSMBuffer;

    D3D12_VIEWPORT mShadowMapViewport;
};