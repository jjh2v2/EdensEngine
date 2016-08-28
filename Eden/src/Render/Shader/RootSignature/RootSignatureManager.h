#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Render/DirectX/D3D12Helper.h"

enum RootSignatureType
{
	RootSignatureType_GBuffer = 0,
	NumRootSignatures
};

struct RootSignatureInfo
{
	CD3DX12_ROOT_SIGNATURE_DESC Desc;
	ID3DBlob* RootSignatureBlob;
	ID3DBlob* Error;
	ID3D12RootSignature* RootSignature;
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