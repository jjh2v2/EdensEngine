#include "Render/Shader/RootSignature/RootSignatureManager.h"

//TDA: Serialize root signatures to files
RootSignatureManager::RootSignatureManager(ID3D12Device *device)
{
	//D3D notes say that performance is best if rangers are ordered from most frequent to least frequently changed

	//GBuffer
	CD3DX12_DESCRIPTOR_RANGE ranges[4];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); //3 frequently changing textures, t0 - t2
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); //1 frequently changing constant buffer, the per-object material buffer, at b1
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 per-frame constant buffer, at b0
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 3, 0); //3 static samplers

	CD3DX12_ROOT_PARAMETER rootParameters[4];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

	RootSignatureInfo gbufferSignature;
	gbufferSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&gbufferSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &gbufferSignature.RootSignatureBlob, &gbufferSignature.Error));
	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, gbufferSignature.RootSignatureBlob->GetBufferPointer(), gbufferSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&gbufferSignature.RootSignature)));

	mRootSignatures.Add(gbufferSignature);
}

//TDA: need to free up all this shader memory 

RootSignatureManager::~RootSignatureManager()
{

}