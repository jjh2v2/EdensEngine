#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/DirectX/D3D12Helper.h"

enum RootSignatureType
{
	RootSignatureType_GBuffer = 0,
	RootSignatureType_Simple_Color = 1,
    RootSignatureType_Simple_Copy_Scaling = 2,
    RootSignatureType_Clear_Shadow_Partitions = 3,
    RootSignatureType_Calculate_Depth_Buffer_Bounds = 4,
    RootSignatureType_Calculate_Log_Partitions_From_Depth = 5,
    RootSignatureType_Clear_Shadow_Partition_Bounds = 6,
    RootSignatureType_Calculate_Partition_Bounds = 7,
    RootSignatureType_Finalize_Partitions = 8,
    RootSignatureType_ShadowMap = 9,
    RootSignatureType_ShadowMapToEVSM = 10,
    RootSignatureType_GenerateMip = 11,
    RootSignatureType_SkyBox = 12,
    RootSignatureType_LightingMain = 13,
    RootSignatureType_FilterCubemap = 14,
    RootSignatureType_GenerateEnvMap = 15,
    RootSignatureType_ShadowMapToEVSM_Compute = 16,
    RootSignatureType_Initial_Luminance = 17,
    RootSignatureType_Luminance_Downsample = 18,
    RootSignatureType_Bloom_Threshold = 19,
    RootSignatureType_Simple_Blur = 20,
    RootSignatureType_ToneMap_Composite = 21,
    RootSignatureType_Ray_Empty_Local = 22,
    RootSignatureType_Ray_Barycentric_Prepass_Global = 23,
    RootSignatureType_Ray_Shadow_Global = 24,
    RootSignatureType_Ray_Shadow_Blur = 25,
	NumRootSignatures
};

struct RootSignatureInfo
{
	CD3DX12_ROOT_SIGNATURE_DESC Desc;
	ID3DBlob* RootSignatureBlob;
	ID3DBlob* Error;
	ID3D12RootSignature* RootSignature;

	RootSignatureInfo()
	{
		RootSignatureBlob = NULL;
		Error = NULL;
		RootSignature = NULL;
	}
};

class RootSignatureManager
{
public:
	RootSignatureManager(ID3D12Device *device);
	~RootSignatureManager();

	const RootSignatureInfo &GetRootSignature(RootSignatureType signatureType) { return mRootSignatures[signatureType]; }

private:
	DynamicArray<RootSignatureInfo> mRootSignatures;
};