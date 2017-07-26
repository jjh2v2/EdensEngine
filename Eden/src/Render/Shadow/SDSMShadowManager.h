#pragma once

/*
struct SDSMPartitionsConstants
{
    D3DXVECTOR4 mLightSpaceBorder;
    D3DXVECTOR4 mMaxScale;
    float mDilationFactor;
    unsigned int mScatterTileDim;
    unsigned int mReduceTileDim;
    float padding;
};

struct SDSMBoundsFloat
{
    D3DXVECTOR3 minCoord;
    D3DXVECTOR3 maxCoord;
};

struct SDSMPartition
{
    float intervalBegin;
    float intervalEnd;
    D3DXVECTOR3 scale;
    D3DXVECTOR3 bias;
};

struct SDSMPerFrameConstants
{
    D3DXMATRIX mCameraWorldViewProj;
    D3DXMATRIX mCameraWorldView;
    D3DXMATRIX mCameraViewProj;
    D3DXMATRIX mCameraProj;
    D3DXMATRIX mLightWorldViewProj;
    D3DXMATRIX mCameraViewToLightProj;
    D3DXMATRIX mWorldMatrix;
    D3DXMATRIX mViewMatrix;
    D3DXMATRIX mProjMatrix;
    D3DXVECTOR4 mLightDir;
    D3DXVECTOR4 mBlurSizeLightSpace;
    D3DXVECTOR4 mCameraNearFar;
};

struct SDSMPerPartitionPassConstants
{
    unsigned int mRenderPartition;
    unsigned int mAccumulatePartitionBegin;
    unsigned int mAccumulatePartitionCount;
    float padding;
};

struct SDSMBoxBlurConstants
{
    D3DXVECTOR2 mFilterSize;
    unsigned int mPartition;
    unsigned int mDimension;
};

struct SDSMShadowPreferences
{
    SDSMShadowPreferences()
    {
        AlignToFrustum = true;
        UseSoftShadows = true;
        SofteningAmount = 0.08f;
        MaxSofteningFilter = 8.0f;
        ShadowAntiAliasingSamples = 4;	//also in the evsm shader, change there
        ShadowTextureSize = 1024;
        PartitionCount = 4;				//only currently handling exactly 4 in the shaders
    }

    bool AlignToFrustum;
    bool UseSoftShadows;
    float SofteningAmount;
    float MaxSofteningFilter;
    unsigned int ShadowAntiAliasingSamples;
    unsigned int ShadowTextureSize;
    unsigned int PartitionCount;
};
*/
class SDSMShadowManager
{
public:
	// NOTE: tileDim must be a multiple of 32
	SDSMShadowManager(GraphicsManager *graphicsManager, int bins);
	~SDSMShadowManager();

	ID3D11ShaderResourceView* ComputeLogPartitionsFromGBuffer(ID3D11DeviceContext *d3dDeviceContext, unsigned int gbufferTexturesNum, ID3D11ShaderResourceView** gbufferTextures,
		int screenWidth, int screenHeight);

	void RenderShadowMapPartition(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* partitionSRV, unsigned int partitionIndex, 
		D3DXMATRIX &lightView, D3DXMATRIX &lightProj, Scene &scene);

	ID3D11ShaderResourceView *GetShadowMap(uint32 index){return mShadowEVSMTextures[index]->GetShaderResourceView();}

	SDSMShadowPreferences GetPreferences(){return mPreferences;}

	void UpdateShaderConstants(ID3D11DeviceContext *d3dDeviceContext,
		D3DXMATRIX &cameraView, D3DXMATRIX &cameraViewInv,	D3DXMATRIX &cameraProj,	D3DXMATRIX &lightView, D3DXMATRIX &lightProj,
		float cameraNear, float cameraFar, D3DXVECTOR3 &lightDirection);

private:

	void ReducePartitionBounds(ID3D11DeviceContext *d3dDeviceContext, unsigned int gbufferTexturesNum, ID3D11ShaderResourceView** gbufferTextures,
		int screenWidth, int screenHeight);

	void RenderShadowDepth(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* partitionSRV, D3DXMATRIX &lightView, D3DXMATRIX &lightProj, Scene &scene, int partitionIndex);

	void ConvertToEVSM(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* partitionSRV, int partitionIndex);

	void BoxBlur(Direct3DManager *direct3DManager, unsigned int partitionIndex, ID3D11ShaderResourceView* partitionSRV, Scene &scene);

	void BoxBlurPass(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* input, ID3D11RenderTargetView* output, unsigned int partitionIndex,
		ID3D11ShaderResourceView* partitionSRV, const D3D11_VIEWPORT* viewport, unsigned int dimension, Scene &scene);

	SDSMShadowPreferences mPreferences;

	int mBins;

	SDSMPartitionsConstants mCurrentConstants;

	StructuredBuffer<SDSMPartition> mPartitionBuffer;
	StructuredBuffer<SDSMBoundsFloat> mPartitionBounds;

	D3DXVECTOR2 mBlurSizeLightSpace;
	ID3D11Buffer *mSDSMPartitionsConstantsBuffer;
	ID3D11Buffer *mPerFrameConstants;

	ComputeShader* mClearZBoundsCS;
	ComputeShader* mReduceZBoundsFromGBufferCS;
	ComputeShader* mLogPartitionsFromZBoundsCS;
	ComputeShader* mCustomPartitionsCS;
	ComputeShader* mClearPartitionBoundsCS;
	ComputeShader* mReduceBoundsFromGBufferCS;

	MaterialInstance *mShadowMapToEVSMMaterial;
	MaterialInstance *mShadowMapBlurMaterial;

	ID3D11RasterizerState *mShadowRasterizerState;
	ID3D11DepthStencilState *mShadowDepthStencilState;

	DepthTexture *mShadowDepthTexture;
	RenderTexture *mShadowEVSMTextures[4];
	RenderTexture *mShadowEVSMBlurTexture;

	D3D11_VIEWPORT mShadowViewport;
};