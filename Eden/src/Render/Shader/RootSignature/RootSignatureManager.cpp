#include "Render/Shader/RootSignature/RootSignatureManager.h"

//TDA: Serialize root signatures to files, and automate this when I finally get fed up with manually adding them
//TDA: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_root_signature_flags use flags properly
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
		//RootSignatureType_Simple_Copy_Scaling
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
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0); //8 srvs at t0-t6
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0); //2 samplers at s0-s2
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
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);      //source env map, t0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);  //env map sampler, s0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);      //rw filter target, u0

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

    {
        //RootSignatureType_GenerateEnvMap
        CD3DX12_DESCRIPTOR_RANGE range;
        range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);      //rw env target, u0

        CD3DX12_ROOT_PARAMETER rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo generateEnvMapSignature;
        generateEnvMapSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&generateEnvMapSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &generateEnvMapSignature.RootSignatureBlob, &generateEnvMapSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, generateEnvMapSignature.RootSignatureBlob->GetBufferPointer(), generateEnvMapSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&generateEnvMapSignature.RootSignature)));

        mRootSignatures.Add(generateEnvMapSignature);
    }

    {
        //RootSignatureType_ShadowMapToEVSM_Compute
        CD3DX12_DESCRIPTOR_RANGE ranges[4];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, partition index buffer
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 srv, shadowmap, at t0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); //1 srv, partitions, at t1
        ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 uav, evsm target, at u0

        CD3DX12_ROOT_PARAMETER rootParameters[4];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo shadowMapEVSMSignature;
        shadowMapEVSMSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&shadowMapEVSMSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &shadowMapEVSMSignature.RootSignatureBlob, &shadowMapEVSMSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, shadowMapEVSMSignature.RootSignatureBlob->GetBufferPointer(), shadowMapEVSMSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&shadowMapEVSMSignature.RootSignature)));

        mRootSignatures.Add(shadowMapEVSMSignature);
    }

    {
        //RootSignatureType_Initial_Luminance
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 srv, hdrTexture, at t0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); //1 uav, lum output, at u1

        CD3DX12_ROOT_PARAMETER rootParameters[2];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo initialLuminanceSignature;
        initialLuminanceSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&initialLuminanceSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &initialLuminanceSignature.RootSignatureBlob, &initialLuminanceSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, initialLuminanceSignature.RootSignatureBlob->GetBufferPointer(), initialLuminanceSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&initialLuminanceSignature.RootSignature)));

        mRootSignatures.Add(initialLuminanceSignature);
    }

    {
        //RootSignatureType_Luminance_Downsample
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0); //2 uav, lum in and out, at u0-1
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, lum buffer
        
        CD3DX12_ROOT_PARAMETER rootParameters[2];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo luminanceDownsampleSignature;
        luminanceDownsampleSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&luminanceDownsampleSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &luminanceDownsampleSignature.RootSignatureBlob, &luminanceDownsampleSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, luminanceDownsampleSignature.RootSignatureBlob->GetBufferPointer(), luminanceDownsampleSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&luminanceDownsampleSignature.RootSignature)));

        mRootSignatures.Add(luminanceDownsampleSignature);
    }

    {
        //RootSignatureType_Bloom_Threshold
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);      //HDR and Lum textures, t0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);  //hdr sampler, s0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);      //threshold buffer, b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo bloomThresholdSignature;
        bloomThresholdSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&bloomThresholdSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &bloomThresholdSignature.RootSignatureBlob, &bloomThresholdSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, bloomThresholdSignature.RootSignatureBlob->GetBufferPointer(), bloomThresholdSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&bloomThresholdSignature.RootSignature)));

        mRootSignatures.Add(bloomThresholdSignature);
    }

    {
        //RootSignatureType_Simple_Blur
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 texture, t0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo simpleBlurSignature;
        simpleBlurSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&simpleBlurSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &simpleBlurSignature.RootSignatureBlob, &simpleBlurSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, simpleBlurSignature.RootSignatureBlob->GetBufferPointer(), simpleBlurSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&simpleBlurSignature.RootSignature)));

        mRootSignatures.Add(simpleBlurSignature);
    }

    {
        //RootSignatureType_ToneMap_Composite
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0); //3 texture, t0-t3
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo toneMapSignature;
        toneMapSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&toneMapSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &toneMapSignature.RootSignatureBlob, &toneMapSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, toneMapSignature.RootSignatureBlob->GetBufferPointer(), toneMapSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&toneMapSignature.RootSignature)));

        mRootSignatures.Add(toneMapSignature);
    }

    {
        //RootSignatureType_Ray_Empty_Local
        RootSignatureInfo raySignature;
        raySignature.Desc.Init(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&raySignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &raySignature.RootSignatureBlob, &raySignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, raySignature.RootSignatureBlob->GetBufferPointer(), raySignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&raySignature.RootSignature)));

        mRootSignatures.Add(raySignature);
    }

    {
        //RootSignatureType_Ray_Barycentric_Prepass_Global
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 uav, output of ray trace
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 srv, acceleration structure
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv, camera matrices

        CD3DX12_ROOT_PARAMETER rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(3, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo rayGenSignature;
        rayGenSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&rayGenSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &rayGenSignature.RootSignatureBlob, &rayGenSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, rayGenSignature.RootSignatureBlob->GetBufferPointer(), rayGenSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rayGenSignature.RootSignature)));

        mRootSignatures.Add(rayGenSignature);
    }

    {
        //RootSignatureType_Ray_Shadow_Global
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 uav, output of ray trace
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); //2 srvs, acceleration structure and depth buffer
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv, camera matrices

        CD3DX12_ROOT_PARAMETER rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(3, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo rayGenSignature;
        rayGenSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_NONE);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&rayGenSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &rayGenSignature.RootSignatureBlob, &rayGenSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, rayGenSignature.RootSignatureBlob->GetBufferPointer(), rayGenSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rayGenSignature.RootSignature)));

        mRootSignatures.Add(rayGenSignature);
    }

    {
        //RootSignatureType_Ray_Shadow_Blur
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 texture, t0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo blurSignature;
        blurSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&blurSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &blurSignature.RootSignatureBlob, &blurSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, blurSignature.RootSignatureBlob->GetBufferPointer(), blurSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&blurSignature.RootSignature)));

        mRootSignatures.Add(blurSignature);
    }

    {
        //RootSignatureType_Luminance_Histogram
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, histogram info
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); //1 srv, hdr input, at t0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); //1 uav, histogram target, at u0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo luminanceSignature;
        luminanceSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&luminanceSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &luminanceSignature.RootSignatureBlob, &luminanceSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, luminanceSignature.RootSignatureBlob->GetBufferPointer(), luminanceSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&luminanceSignature.RootSignature)));

        mRootSignatures.Add(luminanceSignature);
    }

    {
        //RootSignatureType_Luminance_Histogram_Average
        CD3DX12_DESCRIPTOR_RANGE ranges[2];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, histogram info
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0); //2 uav, histogram and average target, at u0-1

        CD3DX12_ROOT_PARAMETER rootParameters[2];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo luminanceSignature;
        luminanceSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&luminanceSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &luminanceSignature.RootSignatureBlob, &luminanceSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, luminanceSignature.RootSignatureBlob->GetBufferPointer(), luminanceSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&luminanceSignature.RootSignature)));

        mRootSignatures.Add(luminanceSignature);
    }

    {
        //RootSignatureType_SDSM_Accumulation
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0); //7 srvs at t0-t6
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); //1 sampler at s0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0, shadow buffer

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

        RootSignatureInfo sdsmSignature;
        sdsmSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&sdsmSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &sdsmSignature.RootSignatureBlob, &sdsmSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, sdsmSignature.RootSignatureBlob->GetBufferPointer(), sdsmSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&sdsmSignature.RootSignature)));

        mRootSignatures.Add(sdsmSignature);
    }

    {
        //RootSignatureType_Water
        CD3DX12_DESCRIPTOR_RANGE ranges[3];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 cbv at b0
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0); //1 textures at t0
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 4, 0); //1 sampler at s0

        CD3DX12_ROOT_PARAMETER rootParameters[3];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

        RootSignatureInfo waterSignature;
        waterSignature.Desc.Init(_countof(rootParameters), rootParameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Direct3DUtils::ThrowIfHRESULTFailed(D3D12SerializeRootSignature(&waterSignature.Desc, D3D_ROOT_SIGNATURE_VERSION_1, &waterSignature.RootSignatureBlob, &waterSignature.Error));
        Direct3DUtils::ThrowIfHRESULTFailed(device->CreateRootSignature(0, waterSignature.RootSignatureBlob->GetBufferPointer(), waterSignature.RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&waterSignature.RootSignature)));

        mRootSignatures.Add(waterSignature);
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