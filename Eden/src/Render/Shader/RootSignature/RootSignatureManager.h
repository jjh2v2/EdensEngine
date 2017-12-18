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