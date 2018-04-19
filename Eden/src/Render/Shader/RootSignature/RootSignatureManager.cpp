#include "Render/Shader/RootSignature/RootSignatureManager.h"

//TDA: Serialize root signatures to files, and automate this when I finally get fed up with manually adding them
RootSignatureManager::RootSignatureManager(ID3D12Device *device)
{
	//Performance is best if ranges are ordered from most frequent to least frequently changed
	{
		//RootSignatureType_GBuffer
		CD3DX12_DESCRIPTOR_RANGE ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); //3 frequently changing textures, t0 - t2
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); //1 frequently changing constant buffer, the per-object material buffer, at b1
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 per-frame constant buffer, at b0
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 3, 0); //3 samplers

		CD3DX12_ROOT_PARAMETER rootParameters[4];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
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
    
	{
		//RootSignatureType_Simple_Copy
		CD3DX12_DESCRIPTOR_RANGE ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 texture, t0
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler

		CD3DX12_ROOT_PARAMETER rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		RootSignatureInfo simpleCopySignature;
		simpleCopySignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&simpleCopySignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &simpleCopySignature.RootSignatureBlob, &simpleCopySignature.Error));
		Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, simpleCopySignature.RootSignatureBlob->GetBufferPointer(), simpleCopySignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&simpleCopySignature.RootSignature)));

		mRootSignatures.Add(simpleCopySignature);
	}

    {
        //RootSignatureType_Clear_Shadow_Partitions
        CD3DX12_DESCRIPTOR_RANGE ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 buffer, u0

        CD3DX12_ROOT_PARAMETER rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo clearShadowPartitionSignature;
        clearShadowPartitionSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&clearShadowPartitionSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &clearShadowPartitionSignature.RootSignatureBlob, &clearShadowPartitionSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, clearShadowPartitionSignature.RootSignatureBlob->GetBufferPointer(), clearShadowPartitionSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&clearShadowPartitionSignature.RootSignature)));

        mRootSignatures.Add(clearShadowPartitionSignature);
    }

    {
        //RootSignatureType_Calculate_Depth_Buffer_Bounds
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 buffer, u0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 texture (depth buffer), t0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv, b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo calculateDepthBoundsSignature;
        calculateDepthBoundsSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&calculateDepthBoundsSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &calculateDepthBoundsSignature.RootSignatureBlob, &calculateDepthBoundsSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, calculateDepthBoundsSignature.RootSignatureBlob->GetBufferPointer(), calculateDepthBoundsSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&calculateDepthBoundsSignature.RootSignature)));

        mRootSignatures.Add(calculateDepthBoundsSignature);
    }

    {
        //RootSignatureType_Calculate_Log_Partitions_From_Depth
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 buffer, u0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv, b0

        CD3DX12_ROOT_PARAMETER rootParameters[2];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo logPartitionsFromDepthSignature;
        logPartitionsFromDepthSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&logPartitionsFromDepthSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &logPartitionsFromDepthSignature.RootSignatureBlob, &logPartitionsFromDepthSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, logPartitionsFromDepthSignature.RootSignatureBlob->GetBufferPointer(), logPartitionsFromDepthSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&logPartitionsFromDepthSignature.RootSignature)));

        mRootSignatures.Add(logPartitionsFromDepthSignature);
    }

    {
        //RootSignatureType_Clear_Shadow_Partition_Bounds
        CD3DX12_DESCRIPTOR_RANGE ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); //1 buffer, u1

        CD3DX12_ROOT_PARAMETER rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo clearShadowPartitionBoundsSignature;
        clearShadowPartitionBoundsSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&clearShadowPartitionBoundsSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &clearShadowPartitionBoundsSignature.RootSignatureBlob, &clearShadowPartitionBoundsSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, clearShadowPartitionBoundsSignature.RootSignatureBlob->GetBufferPointer(), clearShadowPartitionBoundsSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&clearShadowPartitionBoundsSignature.RootSignature)));

        mRootSignatures.Add(clearShadowPartitionBoundsSignature);
    }

    {
        //RootSignatureType_Calculate_Partition_Bounds
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); //1 buffer, u1
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); //2 textures, depth texture and partitions
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv, b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo calculatePartitionBoundsSignature;
        calculatePartitionBoundsSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&calculatePartitionBoundsSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &calculatePartitionBoundsSignature.RootSignatureBlob, &calculatePartitionBoundsSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, calculatePartitionBoundsSignature.RootSignatureBlob->GetBufferPointer(), calculatePartitionBoundsSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&calculatePartitionBoundsSignature.RootSignature)));

        mRootSignatures.Add(calculatePartitionBoundsSignature);
    }
    
    {
        //RootSignatureType_Finalize_Partitions
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 buffer, u0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); //1 buffer, partition bounds
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv, b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo finalizePartitionsSignature;
        finalizePartitionsSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&finalizePartitionsSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &finalizePartitionsSignature.RootSignatureBlob, &finalizePartitionsSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, finalizePartitionsSignature.RootSignatureBlob->GetBufferPointer(), finalizePartitionsSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&finalizePartitionsSignature.RootSignature)));

        mRootSignatures.Add(finalizePartitionsSignature);
    }

    {
        //RootSignatureType_ShadowMap
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); //1 cbv at b1, matrix buffer
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, partition index buffer
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 srv, partitions, at t0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_VERTEX);

        RootSignatureInfo shadowMapSignature;
        shadowMapSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&shadowMapSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &shadowMapSignature.RootSignatureBlob, &shadowMapSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, shadowMapSignature.RootSignatureBlob->GetBufferPointer(), shadowMapSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&shadowMapSignature.RootSignature)));

        mRootSignatures.Add(shadowMapSignature);
    }

    {
        //RootSignatureType_ShadowMapToEVSM
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, partition index buffer
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 srv, shadowmap, at t0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //1 srv, partitions, at t1

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo shadowMapEVSMSignature;
        shadowMapEVSMSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&shadowMapEVSMSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &shadowMapEVSMSignature.RootSignatureBlob, &shadowMapEVSMSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, shadowMapEVSMSignature.RootSignatureBlob->GetBufferPointer(), shadowMapEVSMSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&shadowMapEVSMSignature.RootSignature)));

        mRootSignatures.Add(shadowMapEVSMSignature);
    }

    {
        //RootSignatureType_GenerateMip
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);      //destination mip, u0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);  //1 sampler at s0, mip sampler
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);      //source mip, t0

        CD3DX12_ROOT_PARAMETER rootParameters[4];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_ALL);                //mip info, b0
        rootParameters[2].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);    
        rootParameters[3].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo generateMipSignature;
        generateMipSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&generateMipSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &generateMipSignature.RootSignatureBlob, &generateMipSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, generateMipSignature.RootSignatureBlob->GetBufferPointer(), generateMipSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&generateMipSignature.RootSignature)));

        mRootSignatures.Add(generateMipSignature);
    }

    {
        //RootSignatureType_SkyBox
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); //2 environment srvs at t0-t1
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler at s0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, skybox buffer

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo skyBoxSignature;
        skyBoxSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&skyBoxSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &skyBoxSignature.RootSignatureBlob, &skyBoxSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, skyBoxSignature.RootSignatureBlob->GetBufferPointer(), skyBoxSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&skyBoxSignature.RootSignature)));

        mRootSignatures.Add(skyBoxSignature);
    }

    {
        //RootSignatureType_LightingMain
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 0); //11 srvs at t0-t10
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0); //2 samplers at s0-s1
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, lighting buffer

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo lightingMainSignature;
        lightingMainSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&lightingMainSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &lightingMainSignature.RootSignatureBlob, &lightingMainSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, lightingMainSignature.RootSignatureBlob->GetBufferPointer(), lightingMainSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&lightingMainSignature.RootSignature)));

        mRootSignatures.Add(lightingMainSignature);
    }

    {
        //RootSignatureType_FilterCubemap
        CD3DX12_DESCRIPTOR_RANGE ranges[4];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);      //source env map, t0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);  //env map sampler, s0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);      //rw filter target, u0
       //ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);      //filter info, b0

        CD3DX12_ROOT_PARAMETER rootParameters[4];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[3].InitAsConstants(12, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo filterCubeMapSignature;
        filterCubeMapSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&filterCubeMapSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &filterCubeMapSignature.RootSignatureBlob, &filterCubeMapSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, filterCubeMapSignature.RootSignatureBlob->GetBufferPointer(), filterCubeMapSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&filterCubeMapSignature.RootSignature)));

        mRootSignatures.Add(filterCubeMapSignature);
    }
}

RootSignatureManager::~RootSignatureManager()
{
	for (uint32 i = 0; i < mRootSignatures.CurrentSize(); i++)
	{
		if (mRootSignatures[i].RootSignature)
		{
			mRootSignatures[i].RootSignature->Release();
			mRootSignatures[i].RootSignature = NULL;
		}

		if (mRootSignatures[i].RootSignatureBlob)
		{
			mRootSignatures[i].RootSignatureBlob->Release();
			mRootSignatures[i].RootSignatureBlob = NULL;
		}

		if (mRootSignatures[i].Error)
		{
			mRootSignatures[i].Error->Release();
			mRootSignatures[i].Error = NULL;
		}
	}

	mRootSignatures.Clear();
}