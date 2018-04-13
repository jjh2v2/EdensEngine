#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/DirectX/D3D12Helper.h"

enum RootSignatureType
{
	RootSignatureType_GBuffer = 0,
	RootSignatureType_Simple_Color = 1,
	RootSignatureType_Simple_Copy = 2,
    RootSignatureType_Clear_Shadow_Partitions = 3,
    RootSignatureType_Calculate_Depth_Buffer_Bounds = 4,
    RootSignatureType_Calculate_Log_Partitions_From_Depth = 5,
    RootSignatureType_Clear_Shadow_Partition_Bounds = 6,
    RootSignatureType_Calculate_Partition_Bounds = 7,
    RootSignatureType_Finalize_Partitions = 8,
    RootSignatureType_ShadowMap = 9,
    RootSignatureType_ShadowMapToEVSM = 10,
    RootSignatureType_GenerateMip = 11,
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