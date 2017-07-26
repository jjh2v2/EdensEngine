#include "Render/Shadow/SDSMShadowManager.h"
#include <d3d11.h>
#include <d3dx11.h>
#include <assert.h>
#include <limits>
#include <sstream>


static void UnbindResources(ID3D11DeviceContext *d3dDeviceContext)
{
	ID3D11ShaderResourceView *dummySRV[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	d3dDeviceContext->CSSetShaderResources(0, 8, dummySRV);
	ID3D11UnorderedAccessView *dummyUAV[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	d3dDeviceContext->CSSetUnorderedAccessViews(0, 8, dummyUAV, 0);
}


SDSMShadowManager::SDSMShadowManager(GraphicsManager *graphicsManager, int bins)
	: mBins(bins)
	, mPartitionBuffer(graphicsManager->GetDevice(), mPreferences.PartitionCount)
	, mPartitionBounds(graphicsManager->GetDevice(), mPreferences.PartitionCount)
{	
	std::string binStr;
	{
		std::ostringstream oss;
		oss << mBins;
		binStr = oss.str();
	}
	std::string partitionsStr;
	{
		std::ostringstream oss;
		oss << mPreferences.PartitionCount;
		partitionsStr = oss.str();
	}

	ID3D11Device *device = graphicsManager->GetDevice();

	D3D10_SHADER_MACRO defines[] = {{"BINS", binStr.c_str()}, {"PARTITIONS", partitionsStr.c_str()},{0, 0}};

	mClearZBoundsCS = new ComputeShader(device, L"../EdensEngine/data/HLSL/Compute/LogPartitions.hlsl", "ClearZBounds", defines);
	mReduceZBoundsFromGBufferCS = new ComputeShader(device, L"../EdensEngine/data/HLSL/Compute/LogPartitions.hlsl", "ReduceZBoundsFromGBuffer", defines);

	mLogPartitionsFromZBoundsCS = new ComputeShader(device, L"../EdensEngine/data/HLSL/Compute/LogPartitions.hlsl", "ComputeLogPartitionsFromZBounds", defines);        
	mCustomPartitionsCS = new ComputeShader(device, L"../EdensEngine/data/HLSL/Compute/LogPartitions.hlsl", "ComputeCustomPartitions", defines);

	mClearPartitionBoundsCS = new ComputeShader(device, L"../EdensEngine/data/HLSL/Compute/LogPartitions.hlsl", "ClearPartitionBounds", defines);
	mReduceBoundsFromGBufferCS = new ComputeShader(device, L"../EdensEngine/data/HLSL/Compute/LogPartitions.hlsl", "ReduceBoundsFromGBuffer", defines);

	CD3D11_BUFFER_DESC partitionConstantBufferDesc(sizeof(SDSMPartitionsConstants), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	device->CreateBuffer(&partitionConstantBufferDesc, 0, &mSDSMPartitionsConstantsBuffer);

	CD3D11_BUFFER_DESC perFrameConstantBufferDesc(sizeof(SDSMPerFrameConstants), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC,	D3D11_CPU_ACCESS_WRITE);
	device->CreateBuffer(&perFrameConstantBufferDesc, 0, &mPerFrameConstants);

	mCurrentConstants.mScatterTileDim = 64;
	mCurrentConstants.mReduceTileDim = 128;
	mCurrentConstants.mDilationFactor = 0.01f;

	mShadowMapToEVSMMaterial = graphicsManager->GetMaterialManager()->GetShadowMapToEVSMMaterial();
	mShadowMapBlurMaterial = graphicsManager->GetMaterialManager()->GetBoxBlurMaterial();

	CD3D11_RASTERIZER_DESC rasterDesc(D3D11_DEFAULT);
	rasterDesc.MultisampleEnable = true;
	rasterDesc.DepthClipEnable = false;
	device->CreateRasterizerState(&rasterDesc, &mShadowRasterizerState);

	CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(D3D11_DEFAULT);
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	device->CreateDepthStencilState(&depthStencilDesc, &mShadowDepthStencilState);

	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = mPreferences.ShadowAntiAliasingSamples;
	sampleDesc.Quality = 0;
	mShadowDepthTexture = new DepthTexture(device, mPreferences.ShadowTextureSize, mPreferences.ShadowTextureSize, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, sampleDesc);

	for (uint32 i = 0; i < 4; i++)
	{
		mShadowEVSMTextures[i] = new RenderTexture(device, mPreferences.ShadowTextureSize, mPreferences.ShadowTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT,
			D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 1);
	}
	
	mShadowEVSMBlurTexture = new RenderTexture(device, mPreferences.ShadowTextureSize, mPreferences.ShadowTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 1, 1);

	mShadowViewport.Width = (float)mPreferences.ShadowTextureSize;
	mShadowViewport.Height = (float)mPreferences.ShadowTextureSize;
	mShadowViewport.MinDepth = 0.0f;
	mShadowViewport.MaxDepth = 1.0f;
	mShadowViewport.TopLeftX = 0.0f;
	mShadowViewport.TopLeftY = 0.0f;

}


SDSMShadowManager::~SDSMShadowManager()
{
	mSDSMPartitionsConstantsBuffer->Release();
	mSDSMPartitionsConstantsBuffer = NULL;

	mPerFrameConstants->Release();
	mPerFrameConstants = NULL;

	mShadowRasterizerState->Release();
	mShadowRasterizerState = NULL;

	mShadowDepthStencilState->Release();
	mShadowDepthStencilState = NULL;

	mShadowDepthTexture->Release();
	delete mShadowDepthTexture;
	mShadowDepthTexture = NULL;

	for (uint32 i = 0; i < 4; i++)
	{
		delete mShadowEVSMTextures[i];
		mShadowEVSMTextures[i] = NULL;
	}

	delete mShadowEVSMBlurTexture;
	mShadowEVSMBlurTexture = NULL;

	delete mClearPartitionBoundsCS;
	delete mReduceBoundsFromGBufferCS;

	delete mCustomPartitionsCS;
	delete mLogPartitionsFromZBoundsCS;

	delete mClearZBoundsCS;
	delete mReduceZBoundsFromGBufferCS;
}


void SDSMShadowManager::UpdateShaderConstants(ID3D11DeviceContext *d3dDeviceContext, 
	D3DXMATRIX &cameraView, D3DXMATRIX &cameraViewInv,	D3DXMATRIX &cameraProj,	D3DXMATRIX &lightView, D3DXMATRIX &lightProj, float cameraNear, float cameraFar, D3DXVECTOR3 &lightDirection)
{
	const float maxFloat = FLT_MAX;
	mBlurSizeLightSpace = D3DXVECTOR2(0.0f, 0.0f);
	D3DXVECTOR3 maxPartitionScale(maxFloat, maxFloat, maxFloat);
	if (mPreferences.UseSoftShadows) 
	{
		mBlurSizeLightSpace.x = mPreferences.SofteningAmount * 0.5f * lightProj._11;
		mBlurSizeLightSpace.y = mPreferences.SofteningAmount * 0.5f * lightProj._22;

		float maxBlurLightSpace = mPreferences.MaxSofteningFilter / ((float)mPreferences.ShadowTextureSize);
		maxPartitionScale.x = maxBlurLightSpace / mBlurSizeLightSpace.x;
		maxPartitionScale.y = maxBlurLightSpace / mBlurSizeLightSpace.y;
	}
	D3DXVECTOR3 partitionBorderLightSpace(mBlurSizeLightSpace.x, mBlurSizeLightSpace.y, 1.0f);
	partitionBorderLightSpace.z *= lightProj._33;

	mCurrentConstants.mLightSpaceBorder = D3DXVECTOR4(partitionBorderLightSpace, 0.0f);
	mCurrentConstants.mMaxScale = D3DXVECTOR4(maxPartitionScale, 0.0f);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	d3dDeviceContext->Map(mSDSMPartitionsConstantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	SDSMPartitionsConstants *data = (SDSMPartitionsConstants*)mappedResource.pData;
	*data = mCurrentConstants;
	d3dDeviceContext->Unmap(mSDSMPartitionsConstantsBuffer, 0);

	D3DXMATRIX modelMatrix, positionMatrix, rotationMatrix, scalarMatrix;
	D3DXMatrixIdentity(&positionMatrix);
	D3DXMatrixIdentity(&rotationMatrix);
	D3DXMatrixIdentity(&scalarMatrix);
	D3DXMatrixIdentity(&modelMatrix);

	D3DXVECTOR3 lightDirectionView;
	D3DXVec3TransformNormal(&lightDirectionView, &lightDirection, &cameraView);

	D3DXMATRIXA16 cameraViewProj = cameraView * cameraProj;
	D3DXMATRIXA16 lightViewProj = lightView * lightProj;
	D3DXMATRIXA16 lightWorldViewProj = modelMatrix * lightViewProj;

	d3dDeviceContext->Map(mPerFrameConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	SDSMPerFrameConstants* constants = (SDSMPerFrameConstants*)mappedResource.pData;

	constants->mCameraWorldViewProj = modelMatrix * cameraViewProj;
	constants->mCameraWorldView = modelMatrix * cameraView;
	constants->mCameraViewProj = cameraViewProj;
	constants->mCameraProj = cameraProj;
	constants->mLightWorldViewProj = lightWorldViewProj;
	constants->mCameraViewToLightProj = cameraViewInv * lightViewProj;
	constants->mWorldMatrix = modelMatrix;
	constants->mViewMatrix = cameraView;
	constants->mProjMatrix = cameraProj;
	constants->mLightDir = D3DXVECTOR4(lightDirectionView, 0.0f);
	constants->mBlurSizeLightSpace = D3DXVECTOR4(mBlurSizeLightSpace.x, mBlurSizeLightSpace.y, 0.0f, 0.0f);
	constants->mCameraNearFar = D3DXVECTOR4(cameraNear, cameraFar, 0.0f, 0.0f);

	d3dDeviceContext->Unmap(mPerFrameConstants, 0);
}

ID3D11ShaderResourceView* SDSMShadowManager::ComputeLogPartitionsFromGBuffer(ID3D11DeviceContext *d3dDeviceContext, unsigned int gbufferTexturesNum, ID3D11ShaderResourceView** gbufferTextures,
	int screenWidth, int screenHeight)
{
	d3dDeviceContext->CSSetConstantBuffers(0, 1, &mPerFrameConstants);
	d3dDeviceContext->CSSetConstantBuffers(1, 1, &mSDSMPartitionsConstantsBuffer);

	ID3D11UnorderedAccessView* partitionUAV = mPartitionBuffer.GetUnorderedAccess();    
	d3dDeviceContext->CSSetShader(mClearZBoundsCS->GetShader(), 0, 0);
	d3dDeviceContext->CSSetUnorderedAccessViews(6, 1, &partitionUAV, 0);
	d3dDeviceContext->Dispatch(1, 1, 1);

	int dispatchWidth = (screenWidth + mCurrentConstants.mReduceTileDim - 1) / mCurrentConstants.mReduceTileDim;
	int dispatchHeight = (screenHeight + mCurrentConstants.mReduceTileDim - 1) / mCurrentConstants.mReduceTileDim;

	d3dDeviceContext->CSSetShader(mReduceZBoundsFromGBufferCS->GetShader(), 0, 0);
	d3dDeviceContext->CSSetShaderResources(0, gbufferTexturesNum, gbufferTextures);
	d3dDeviceContext->CSSetUnorderedAccessViews(6, 1, &partitionUAV, 0);
	d3dDeviceContext->Dispatch(dispatchWidth, dispatchHeight, 1);

	d3dDeviceContext->CSSetShader(mLogPartitionsFromZBoundsCS->GetShader(), 0, 0);
	d3dDeviceContext->CSSetUnorderedAccessViews(6, 1, &partitionUAV, 0);
	d3dDeviceContext->Dispatch(1, 1, 1);

	ReducePartitionBounds(d3dDeviceContext, gbufferTexturesNum, gbufferTextures, screenWidth, screenHeight);

	return mPartitionBuffer.GetShaderResource();
}


void SDSMShadowManager::ReducePartitionBounds(ID3D11DeviceContext *d3dDeviceContext, unsigned int gbufferTexturesNum, ID3D11ShaderResourceView** gbufferTextures,
	int screenWidth, int screenHeight)
{
	d3dDeviceContext->CSSetConstantBuffers(0, 1, &mPerFrameConstants);
	d3dDeviceContext->CSSetConstantBuffers(1, 1, &mSDSMPartitionsConstantsBuffer);

	int dispatchWidth = (screenWidth + mCurrentConstants.mReduceTileDim - 1) / mCurrentConstants.mReduceTileDim;
	int dispatchHeight = (screenHeight + mCurrentConstants.mReduceTileDim - 1) / mCurrentConstants.mReduceTileDim;

	ID3D11UnorderedAccessView* partitionBoundsUAV = mPartitionBounds.GetUnorderedAccess();   
	d3dDeviceContext->CSSetShader(mClearPartitionBoundsCS->GetShader(), 0, 0);
	d3dDeviceContext->CSSetUnorderedAccessViews(7, 1, &partitionBoundsUAV, 0);
	d3dDeviceContext->Dispatch(1, 1, 1);

	UnbindResources(d3dDeviceContext);

	ID3D11ShaderResourceView* partitionSRV = mPartitionBuffer.GetShaderResource();
	d3dDeviceContext->CSSetShader(mReduceBoundsFromGBufferCS->GetShader(), 0, 0);
	d3dDeviceContext->CSSetShaderResources(0, gbufferTexturesNum, gbufferTextures);
	d3dDeviceContext->CSSetShaderResources(6, 1, &partitionSRV);
	d3dDeviceContext->CSSetUnorderedAccessViews(7, 1, &partitionBoundsUAV, 0);
	d3dDeviceContext->Dispatch(dispatchWidth, dispatchHeight, 1);

	UnbindResources(d3dDeviceContext);

	ID3D11ShaderResourceView* partitionBoundsSRV = mPartitionBounds.GetShaderResource();
	ID3D11UnorderedAccessView* partitionUAV = mPartitionBuffer.GetUnorderedAccess();
	d3dDeviceContext->CSSetShader(mCustomPartitionsCS->GetShader(), 0, 0);
	d3dDeviceContext->CSSetShaderResources(7, 1, &partitionBoundsSRV);
	d3dDeviceContext->CSSetUnorderedAccessViews(6, 1, &partitionUAV, 0);
	d3dDeviceContext->Dispatch(1, 1, 1);

	UnbindResources(d3dDeviceContext);
}

void SDSMShadowManager::RenderShadowMapPartition(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* partitionSRV, unsigned int partitionIndex, D3DXMATRIX &lightView, 
	D3DXMATRIX &lightProj, Scene &scene)
{
	// Generate raw depth map
	RenderShadowDepth(direct3DManager, partitionSRV, lightView, lightProj, scene, partitionIndex);

	// Convert single depth map to an EVSM in the proper array slice
	ConvertToEVSM(direct3DManager, partitionSRV, partitionIndex);

	if (mPreferences.UseSoftShadows) 
	{
		BoxBlur(direct3DManager, partitionIndex, partitionSRV, scene);
	}

	direct3DManager->GetDeviceContext()->GenerateMips(mShadowEVSMTextures[partitionIndex]->GetShaderResourceView());
}

void SDSMShadowManager::RenderShadowDepth(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* partitionSRV,
	D3DXMATRIX &lightView, D3DXMATRIX &lightProj, Scene &scene,	int partitionIndex)
{
	direct3DManager->GetDeviceContext()->ClearDepthStencilView(mShadowDepthTexture->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	direct3DManager->GetDeviceContext()->RSSetState(mShadowRasterizerState);
	direct3DManager->GetDeviceContext()->RSSetViewports(1, &mShadowViewport);

	direct3DManager->GetDeviceContext()->OMSetDepthStencilState(mShadowDepthStencilState, 0);
	direct3DManager->GetDeviceContext()->OMSetRenderTargets(0, 0, mShadowDepthTexture->GetDepthStencilView());
	direct3DManager->DisableAlphaBlending();

	D3DXMATRIX lightViewProject = lightView * lightProj;

	scene.RenderToShadowMap(direct3DManager->GetDeviceContext(), partitionSRV, lightViewProject, partitionIndex);

	direct3DManager->ClearDeviceTargetsAndResources();
}

void SDSMShadowManager::ConvertToEVSM(Direct3DManager *direct3DManager,	ID3D11ShaderResourceView* partitionSRV,	int partitionIndex)
{
	direct3DManager->SetupForFullScreenTriangle();
	direct3DManager->UseDefaultRasterState();
	direct3DManager->DisableAlphaBlending();

	direct3DManager->GetDeviceContext()->RSSetViewports(1, &mShadowViewport);

	ID3D11RenderTargetView *target = mShadowEVSMTextures[partitionIndex]->GetRenderTargetView();
	direct3DManager->GetDeviceContext()->OMSetRenderTargets(1, &target, 0);

	ShaderParams shaderParams;
	shaderParams.ShadowMap = mShadowDepthTexture->GetShaderResourceView();
	shaderParams.PartitionResourceView = partitionSRV;
	shaderParams.PartitionIndex = partitionIndex;

	mShadowMapToEVSMMaterial->SetShaderParameters(direct3DManager->GetDeviceContext(), shaderParams);
	mShadowMapToEVSMMaterial->Render(direct3DManager->GetDeviceContext());
	direct3DManager->GetDeviceContext()->Draw(3, 0);

	direct3DManager->ClearDeviceTargetsAndResources();
}

void SDSMShadowManager::BoxBlurPass(Direct3DManager *direct3DManager, ID3D11ShaderResourceView* input, ID3D11RenderTargetView* output, unsigned int partitionIndex,
	ID3D11ShaderResourceView* partitionSRV, const D3D11_VIEWPORT* viewport, unsigned int dimension, Scene &scene)
{
	direct3DManager->SetupForFullScreenTriangle();
	direct3DManager->UseDefaultRasterState();
	direct3DManager->DisableAlphaBlending();
	direct3DManager->GetDeviceContext()->RSSetViewports(1, viewport);

	direct3DManager->GetDeviceContext()->OMSetRenderTargets(1, &output, 0);

	ShaderParams shaderParams;
	shaderParams.BlurTexture = input;
	shaderParams.PartitionResourceView = partitionSRV;
	shaderParams.FilterSize = Vector2::FromD3DVector(mBlurSizeLightSpace);
	shaderParams.BlurDirection = dimension == 0;
	shaderParams.PartitionIndex = partitionIndex;

	mShadowMapBlurMaterial->SetShaderParameters(direct3DManager->GetDeviceContext(), shaderParams);
	mShadowMapBlurMaterial->Render(direct3DManager->GetDeviceContext());
	direct3DManager->GetDeviceContext()->Draw(3, 0);

	direct3DManager->ClearDeviceTargetsAndResources();
}


void SDSMShadowManager::BoxBlur(Direct3DManager *direct3DManager, unsigned int partitionIndex, ID3D11ShaderResourceView* partitionSRV, Scene &scene)
{
	D3D11_TEXTURE2D_DESC textureDesc;
	mShadowEVSMTextures[partitionIndex]->GetRenderTargetTexture()->GetDesc(&textureDesc);

	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(textureDesc.Width);
	viewport.Height = static_cast<float>(textureDesc.Height);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	BoxBlurPass(direct3DManager, mShadowEVSMTextures[partitionIndex]->GetShaderResourceView(0), mShadowEVSMBlurTexture->GetRenderTargetView(0),
		partitionIndex, partitionSRV, &viewport, 0, scene);

	BoxBlurPass(direct3DManager, mShadowEVSMBlurTexture->GetShaderResourceView(0), mShadowEVSMTextures[partitionIndex]->GetRenderTargetView(0),
		partitionIndex, partitionSRV, &viewport, 1, scene);
}
