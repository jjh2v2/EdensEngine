#include "Render/Shader/RootSignature/RootSignatureManager.h"

//TDA: Serialize root signatures to files
RootSignatureManager::RootSignatureManager(ID3D12Device *device)
{
	//D3D notes say that performance is best if ranges are ordered from most frequent to least frequently changed

	{
		//RootSignatureType_GBuffer
		CD3DX12_DESCRIPTOR_RANGE ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); //3 frequently changing textures, t0 - t2
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); //1 frequently changing constant buffer, the per-object material buffer, at b1
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 per-frame constant buffer, at b0
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 3, 0); //3 samplers

		CD3DX12_ROOT_PARAMETER rootParameters[4];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);		//TDA: Any way to | just pixel + vertex together? Answer: need to place deny flags here somewhere
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		RootSignatureInfo gbufferSignature;
		gbufferSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&gbufferSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &gbufferSignature.RootSignatureBlob, &gbufferSignature.Error));
		Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, gbufferSignature.RootSignatureBlob->GetBufferPointer(), gbufferSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&gbufferSignature.RootSignature)));

		mRootSignatures.Add(gbufferSignature);
	}
	
	{
		//RootSignatureType_Simple_Color
		CD3DX12_DESCRIPTOR_RANGE ranges[3];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 texture, t0
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0

		CD3DX12_ROOT_PARAMETER rootParameters[3];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_VERTEX);

		RootSignatureInfo simpleColorSignature;
		simpleColorSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&simpleColorSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &simpleColorSignature.RootSignatureBlob, &simpleColorSignature.Error));
		Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, simpleColorSignature.RootSignatureBlob->GetBufferPointer(), simpleColorSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&simpleColorSignature.RootSignature)));

		mRootSignatures.Add(simpleColorSignature);
	}
}

RootSignatureManager::~RootSignatureManager()
{
	for (uint32 i = 0; i < mRootSignatures.CurrentSize(); i++)
	{
		if (mRootSignatures[i].RootSignature)
		{
			mRootSignatures[i].RootSignature->Release();
		}

		if (mRootSignatures[i].RootSignatureBlob)
		{
			mRootSignatures[i].RootSignatureBlob->Release();
		}

		if (mRootSignatures[i].Error)
		{
			mRootSignatures[i].Error->Release();
		}
	}

	mRootSignatures.Clear();
}