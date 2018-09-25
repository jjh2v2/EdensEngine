#include "Render/Texture/TextureManager.h"
#include "Util/String/StringConverter.h"
#include "Core/Threading/ThreadPoolManager.h"
#include "Render/Texture/Sampler/Sampler.h"
#include "Render/Shader/Definitions/ConstantBufferDefinitions.h"

Texture *TextureManager::mDefaultTexture = NULL;

TextureUpload::TextureUpload()
{
    UploadState = TextureUploadState_Pending;
    TextureToUpload = NULL;
    FilePath = NULL;
    ImageData = NULL;
    UploadFence = 0;
    ReadJob = NULL;
    FinalDesiredResourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

TextureUpload::~TextureUpload()
{
    delete[] FilePath;
    FilePath = NULL;

    delete ImageData;
    ImageData = NULL;

    if (ReadJob)
    {
        delete ReadJob;
        ReadJob = NULL;
    }
}

TextureManager::TextureManager(Direct3DManager *direct3DManager)
{
	mDirect3DManager = direct3DManager;
}

TextureManager::~TextureManager()
{
    //wait until pending uploads are completed before nuking everything. We don't just delete them because they could have jobs mid-process
    while (mTextureUploads.CurrentSize() > 0)
    {
        ProcessCurrentUploads();
    }

	for (uint32 i = 0; i < mTextures.CurrentSize(); i++)
	{
        if (mTextures[i]->GetTextureResource())
        {
            mDirect3DManager->GetContextManager()->GetHeapManager()->FreeSRVDescriptorHeapHandle(mTextures[i]->GetTextureResource()->GetShaderResourceViewHandle());
        }

        delete mTextures[i];
	}

	mTextures.Clear();
}

void TextureManager::LoadTextureManifest()
{
	mManifestLoader.LoadManifest(ApplicationSpecification::TextureManifestFileLocation);

	DynamicArray<std::string, false> &fileNames = mManifestLoader.GetFileNames();
	for (uint32 i = 0; i < fileNames.CurrentSize(); i++)
	{
		size_t lastSlash = fileNames[i].find_last_of("/");
		size_t lastDot = fileNames[i].find_last_of(".");
		std::string justFileName = fileNames[i].substr(lastSlash + 1, (lastDot - lastSlash) - 1);

		TextureLookup textureLookup;
		textureLookup.TextureFilePath = fileNames[i];
		textureLookup.TextureRef = NULL;
		mTextureLookup.insert(std::pair<std::string, TextureLookup>(justFileName, textureLookup));
	}

    mDefaultTexture = GetTexture("Default", false);
}

Texture *TextureManager::GetTexture(const std::string &textureName, bool async /* = true */)
{
    std::lock_guard<std::mutex> lock(mTextureLookupMutex);

	Texture *texture = mTextureLookup[textureName].TextureRef;
	if (!texture)
	{
		WCHAR *convertedString = StringConverter::StringToWCHARAlloc(mTextureLookup[textureName].TextureFilePath);
		texture = LoadTexture(convertedString, async);
		mTextureLookup[textureName].TextureRef = texture;
        mTextures.Add(texture);
	}

	return texture;
}

Texture *TextureManager::LoadTexture(WCHAR *filePath, bool async)
{
    TextureUpload *newTextureUpload = new TextureUpload();
    newTextureUpload->TextureToUpload = new Texture();
    newTextureUpload->UploadState = TextureUploadState_Initialized;
    newTextureUpload->FilePath = filePath;
    newTextureUpload->ImageData = new DirectX::ScratchImage();

    if(async)
    {
        std::lock_guard<std::mutex> lock(mTextureUploadMutex);
        mTextureUploads.Add(newTextureUpload);
        return newTextureUpload->TextureToUpload;
    }

    //synchronous uploads need to lock the upload context and stall for completion
    Direct3DQueueManager *queueManager = mDirect3DManager->GetContextManager()->GetQueueManager();
    UploadContext *uploadContext = mDirect3DManager->GetContextManager()->GetUploadContext();

    ProcessFileRead(newTextureUpload);

    uploadContext->LockUploads();
    TextureBackgroundUpload uploadJob(mDirect3DManager->GetDevice(), queueManager, newTextureUpload);
    uploadJob.ProcessUpload(uploadContext);
    queueManager->GetCopyQueue()->WaitForFenceCPUBlocking(newTextureUpload->UploadFence);
    uploadContext->UnlockUploads();

    uint64 currentUploadFence = queueManager->GetCopyQueue()->PollCurrentFenceValue();
    ProcessTransition(newTextureUpload, currentUploadFence);
    newTextureUpload->TextureToUpload->SetIsReady(true);

    Texture *newTexture = newTextureUpload->TextureToUpload;
    delete newTextureUpload;
    return newTexture;
}

TextureManager::TextureReadJob::TextureReadJob(TextureUpload *textureUpload, TextureManager *textureManager)
{
    mTextureUpload = textureUpload;
    mTextureManager = textureManager;
}

void TextureManager::TextureReadJob::Execute()
{
    mTextureManager->ProcessFileRead(mTextureUpload);
}

void TextureManager::ProcessCurrentUploads()
{
    std::lock_guard<std::mutex> lock(mTextureUploadMutex);

    Direct3DQueueManager *queueManager = mDirect3DManager->GetContextManager()->GetQueueManager();
    uint64 currentUploadFence = queueManager->GetCopyQueue()->PollCurrentFenceValue();

    uint32 numUploads = mTextureUploads.CurrentSize();
    for (uint32 i = 0; i < numUploads; i++)
    {
        TextureUpload *currentUpload = mTextureUploads[i];
        switch (currentUpload->UploadState)
        {
        case TextureUploadState_Initialized:
            currentUpload->UploadState = TextureUploadState_Read;
            break;
        case TextureUploadState_Read:
            currentUpload->UploadState = TextureUploadState_Pending;
            currentUpload->ReadJob = new TextureReadJob(currentUpload, this);
            ThreadPoolManager::GetSingleton()->GetBackgroundThreadPool()->AddSingleJob(currentUpload->ReadJob);
            break;
        case TextureUploadState_Upload:
            currentUpload->UploadState = TextureUploadState_Pending;
            mDirect3DManager->GetContextManager()->GetUploadContext()->AddBackgroundUpload(new TextureBackgroundUpload(mDirect3DManager->GetDevice(), queueManager, currentUpload));
            break;
        case TextureUploadState_Transition:
            ProcessTransition(currentUpload, currentUploadFence);
            break;
        case TextureUploadState_Completed:
            currentUpload->TextureToUpload->SetIsReady(true);
            currentUpload->UploadState = TextureUploadState_Delete;
            break;
        case TextureUploadState_Pending:
            //Background job is currently pending, just wait
            break;
        default:
            Application::Assert(false);
            break;
        }
    }

    for (int32 i = 0; i < mTextureUploads.CurrentSizeSigned(); i++)
    {
        TextureUpload *currentUpload = mTextureUploads[i];

        if (currentUpload->UploadState == TextureUploadState_Delete)
        {
            mTextureUploads.Remove(i);
            delete currentUpload;
            i--;
        }
    }
}

void TextureManager::ProcessCurrentComputeWork()
{
    if (mCubeMapFilters.CurrentSize() == 0 && mGeneratedTextures.CurrentSize() == 0)
    {
        return;
    }

    Direct3DQueue *computeQueue = mDirect3DManager->GetContextManager()->GetQueueManager()->GetComputeQueue();
    uint64 mostRecentComputeFence = computeQueue->PollCurrentFenceValue();
    uint32 numProcessed = 0;

    uint32 numSRVDescs = 0;
    uint32 numSamplerDescs = 0;

    numSRVDescs += mGeneratedTextures.CurrentSize();
    numSamplerDescs += mCubeMapFilters.CurrentSize();

    for (uint32 i = 0; i < mCubeMapFilters.CurrentSize(); i++)
    {
        numSRVDescs += 1 + 6 * mCubeMapFilters[i].NumMips;
    }

    Direct3DHeapManager *heapManager = mDirect3DManager->GetContextManager()->GetHeapManager();
    RenderPassDescriptorHeap *filterSRVDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_TextureProcessing, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mDirect3DManager->GetFrameIndex(), numSRVDescs);
    RenderPassDescriptorHeap *filterSamplerDescHeap = heapManager->GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType_TextureProcessing, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mDirect3DManager->GetFrameIndex(), numSamplerDescs);

    for (int32 i = 0; i < mGeneratedTextures.CurrentSizeSigned(); i++)
    {
        if (mGeneratedTextures[i].GenerationFence == 0 && numProcessed < MAX_ASYNC_COMPUTE_TEXTURES_TO_PROCESS_PER_FRAME)
        {
            ProcessGeneration(mGeneratedTextures[i], filterSRVDescHeap);
            numProcessed++;
        }
        else if (mGeneratedTextures[i].GenerationFence != 0)
        {
            if (mGeneratedTextures[i].GenerationFence <= mostRecentComputeFence)
            {
                mDirect3DManager->GetContextManager()->GetGraphicsContext()->TransitionResource(mGeneratedTextures[i].GenerateTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
                mGeneratedTextures[i].GenerateTarget->SetIsReady(true);
                mGeneratedTextures.Remove(i);
                i--;
            }
        }
    }

    for (int32 i = 0; i < mCubeMapFilters.CurrentSizeSigned(); i++)
    {
        if (mCubeMapFilters[i].CubeMapToFilter->GetIsReady()) //make sure environment texture is fully loaded first
        {
            if (mCubeMapFilters[i].FilterTarget->GetComputeFence() == 0 && numProcessed < MAX_ASYNC_COMPUTE_TEXTURES_TO_PROCESS_PER_FRAME)
            {
                ProcessCubeMapFiltering(mCubeMapFilters[i], filterSRVDescHeap, filterSamplerDescHeap);
                numProcessed++;
            }
            else if(mCubeMapFilters[i].FilterTarget->GetComputeFence() != 0)
            {
                if (mCubeMapFilters[i].FilterTarget->GetComputeFence() <= mostRecentComputeFence)
                {
                    mDirect3DManager->GetContextManager()->GetGraphicsContext()->TransitionResource(mCubeMapFilters[i].FilterTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
                    mCubeMapFilters[i].FilterTarget->SetComputeFence(0);
                    mCubeMapFilters[i].FilterTarget->SetIsReady(true);

                    mCubeMapFilters.Remove(i);
                    i--;
                }
            }
        }
    }
}

void TextureManager::ProcessFileRead(TextureUpload *currentUpload)
{
    HRESULT loadResult = DirectX::LoadFromDDSFile(currentUpload->FilePath, DirectX::DDS_FLAGS_NONE, nullptr, *currentUpload->ImageData);
    assert(loadResult == S_OK);

    const DirectX::TexMetadata& textureMetaData = currentUpload->ImageData->GetMetadata();
    DXGI_FORMAT textureFormat = textureMetaData.format;
    bool is3DTexture = textureMetaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D;

    currentUpload->TextureToUpload->SetDimensions(Vector3((float)textureMetaData.width, (float)textureMetaData.height, (float)textureMetaData.depth));
    currentUpload->TextureToUpload->SetMipCount((uint32)textureMetaData.mipLevels);
    currentUpload->TextureToUpload->SetArraySize((uint32)textureMetaData.arraySize);
    currentUpload->TextureToUpload->SetFormat(textureMetaData.format);
    currentUpload->TextureToUpload->SetIsCubeMap(textureMetaData.IsCubemap());

    if (textureMetaData.IsCubemap())
    {
        //we typically need to filter cube maps
        currentUpload->FinalDesiredResourceStates |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    if (ApplicationSpecification::ForceAllTexturesToSRGB)
    {
        textureFormat = DirectX::MakeSRGB(textureFormat);
    }

    currentUpload->TextureDesc.MipLevels = (uint16)textureMetaData.mipLevels;
    currentUpload->TextureDesc.Format = textureFormat;
    currentUpload->TextureDesc.Width = (uint32)textureMetaData.width;
    currentUpload->TextureDesc.Height = (uint32)textureMetaData.height;
    currentUpload->TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    currentUpload->TextureDesc.DepthOrArraySize = is3DTexture ? uint16(textureMetaData.depth) : uint16(textureMetaData.arraySize);
    currentUpload->TextureDesc.SampleDesc.Count = 1;
    currentUpload->TextureDesc.SampleDesc.Quality = 0;
    currentUpload->TextureDesc.Dimension = is3DTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    currentUpload->TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    currentUpload->TextureDesc.Alignment = 0;

    D3D12_HEAP_PROPERTIES defaultProperties;
    defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultProperties.CreationNodeMask = 0;
    defaultProperties.VisibleNodeMask = 0;

    ID3D12Resource *newTextureResource = NULL;
    mDirect3DManager->GetDevice()->CreateCommittedResource(&defaultProperties, D3D12_HEAP_FLAG_NONE, &currentUpload->TextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&newTextureResource));

    TextureResource *textureGPUResource = new TextureResource(newTextureResource, D3D12_RESOURCE_STATE_COPY_DEST, mDirect3DManager->GetContextManager()->GetHeapManager()->GetNewSRVDescriptorHeapHandle());
    currentUpload->TextureToUpload->SetTextureResource(textureGPUResource);

    D3D12_SHADER_RESOURCE_VIEW_DESC *srvDescPointer = NULL;
    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
    if (textureMetaData.IsCubemap())
    {
        Application::Assert(textureMetaData.arraySize == 6);

        shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
        shaderResourceViewDesc.TextureCube.MipLevels = uint32(textureMetaData.mipLevels);
        shaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        srvDescPointer = &shaderResourceViewDesc;
    }

    mDirect3DManager->GetDevice()->CreateShaderResourceView(newTextureResource, srvDescPointer, currentUpload->TextureToUpload->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle());

    currentUpload->UploadState = TextureUploadState_Upload;
}

TextureManager::TextureBackgroundUpload::TextureBackgroundUpload(ID3D12Device *device, Direct3DQueueManager *queueManager, TextureUpload *textureUpload)
{
    mDevice = device;
    mQueueManager = queueManager;
    mTextureUpload = textureUpload;
}

void TextureManager::TextureBackgroundUpload::ProcessUpload(UploadContext *uploadContext)
{
    uint64 textureMemorySize = 0;
    UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
    UINT64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
    const DirectX::TexMetadata& textureMetaData = mTextureUpload->ImageData->GetMetadata();
    const uint64 numSubResources = textureMetaData.mipLevels * textureMetaData.arraySize;

    mDevice->GetCopyableFootprints(&mTextureUpload->TextureDesc, 0, uint32(numSubResources), 0, mTextureUpload->Layouts, numRows, rowSizesInBytes, &textureMemorySize);

    mTextureUpload->UploadInfo = uploadContext->BeginUpload(textureMemorySize, mQueueManager);
    uint8* uploadMem = reinterpret_cast<uint8*>(mTextureUpload->UploadInfo.CPUAddress);

    for (uint64 arrayIdx = 0; arrayIdx < textureMetaData.arraySize; ++arrayIdx)
    {
        for (uint64 mipIdx = 0; mipIdx < textureMetaData.mipLevels; ++mipIdx)
        {
            const uint64 subResourceIdx = mipIdx + (arrayIdx * textureMetaData.mipLevels);

            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = mTextureUpload->Layouts[subResourceIdx];
            const uint64 subResourceHeight = numRows[subResourceIdx];
            const uint64 subResourcePitch = subResourceLayout.Footprint.RowPitch;
            const uint64 subResourceDepth = subResourceLayout.Footprint.Depth;
            uint8* dstSubResourceMem = reinterpret_cast<uint8*>(uploadMem) + subResourceLayout.Offset;

            for (uint64 z = 0; z < subResourceDepth; ++z)
            {
                const DirectX::Image* subImage = mTextureUpload->ImageData->GetImage(mipIdx, arrayIdx, z);
                Application::Assert(subImage != NULL);
                const uint8* srcSubResourceMem = subImage->pixels;

                for (uint64 y = 0; y < subResourceHeight; ++y)
                {
                    memcpy(dstSubResourceMem, srcSubResourceMem, MathHelper::Min(subResourcePitch, subImage->rowPitch));
                    dstSubResourceMem += subResourcePitch;
                    srcSubResourceMem += subImage->rowPitch;
                }
            }
        }
    }

    for (uint64 subResourceIdx = 0; subResourceIdx < numSubResources; ++subResourceIdx)
    {
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = mTextureUpload->TextureToUpload->GetTextureResource()->GetResource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = uint32(subResourceIdx);
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = mTextureUpload->UploadInfo.Resource;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = mTextureUpload->Layouts[subResourceIdx];
        src.PlacedFootprint.Offset += mTextureUpload->UploadInfo.UploadAddressOffset;
        uploadContext->CopyTextureRegion(&dst, &src);
    }

    mTextureUpload->UploadFence = uploadContext->FlushUpload(mTextureUpload->UploadInfo, mQueueManager);
    mTextureUpload->UploadState = TextureUploadState_Transition;

    mUploadFence = mTextureUpload->UploadFence;
}

void TextureManager::ProcessTransition(TextureUpload *currentUpload, uint64 currentFence)
{
    if (currentUpload->UploadFence <= currentFence)
    {
        mDirect3DManager->GetContextManager()->GetGraphicsContext()->TransitionResource(currentUpload->TextureToUpload->GetTextureResource(), currentUpload->FinalDesiredResourceStates, true);
        currentUpload->UploadState = TextureUploadState_Completed;
    }
}

void TextureManager::ProcessCubeMapFiltering(CubeMapFilterInfo &cubeMapFilterInfo, RenderPassDescriptorHeap *srvHeap, RenderPassDescriptorHeap *samplerHeap)
{
    Application::Assert(cubeMapFilterInfo.CubeMapToFilter->GetIsReady());

    Vector4 targets[6] =
    {
        Vector4(1.0f,  0.0f,  0.0f, 0.0f),  // +X
        Vector4(-1.0f, 0.0f,  0.0f, 0.0f),  // -X
        Vector4(0.0f,  1.0f,  0.0f, 0.0f),  // +Y
        Vector4(0.0f, -1.0f,  0.0f, 0.0f),  // -Y
        Vector4(0.0f,  0.0f,  1.0f, 0.0f),  // +Z
        Vector4(0.0f,  0.0f, -1.0f, 0.0f)   // -Z
    };

    Vector4 ups[6] =
    {
        Vector4(0.0f, 1.0f,  0.0f, 0.0f),  // +X
        Vector4(0.0f, 1.0f,  0.0f, 0.0f),  // -X
        Vector4(0.0f, 0.0f, -1.0f, 0.0f),  // +Y
        Vector4(0.0f, 0.0f,  1.0f, 0.0f),  // -Y
        Vector4(0.0f, 1.0f,  0.0f, 0.0f),  // +Z
        Vector4(0.0f, 1.0f,  0.0f, 0.0f)   // -Z
    };
    
    ComputeContext *computeContext = mDirect3DManager->GetContextManager()->GetComputeContext();
    computeContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap->GetHeap());
    computeContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerHeap->GetHeap());

    computeContext->SetPipelineState(cubeMapFilterInfo.EnvironmentFilterShader);
    computeContext->SetRootSignature(cubeMapFilterInfo.EnvironmentFilterShader->GetRootSignature());

    DescriptorHeapHandle envMapHandle = srvHeap->GetHeapHandleBlock(1);
    DescriptorHeapHandle envMapSamplerHandle = samplerHeap->GetHeapHandleBlock(1);
    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, envMapHandle.GetCPUHandle(), cubeMapFilterInfo.CubeMapToFilter->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, envMapSamplerHandle.GetCPUHandle(), cubeMapFilterInfo.EnvironmentSampler->GetSamplerHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    computeContext->SetDescriptorTable(0, envMapHandle.GetGPUHandle());
    computeContext->SetDescriptorTable(1, envMapSamplerHandle.GetGPUHandle());

    EnvironmentMapFilterTextureBuffer filterInfo;
    filterInfo.mipCount = (float)cubeMapFilterInfo.NumMips;

    for (uint32 cubeSideIndex = 0; cubeSideIndex < 6; cubeSideIndex++)
    {
        filterInfo.forwardDir = targets[cubeSideIndex];
        filterInfo.upDir = ups[cubeSideIndex];
        filterInfo.sourceDimensions = Vector2((float)cubeMapFilterInfo.DimensionSize, (float)cubeMapFilterInfo.DimensionSize);

        for (uint32 mipIndex = 0; mipIndex < cubeMapFilterInfo.NumMips; mipIndex++)
        {
            filterInfo.mipLevel = (float)(cubeMapFilterInfo.NumMips - mipIndex);
            DescriptorHeapHandle filterTargetHandle = srvHeap->GetHeapHandleBlock(1);
            mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, filterTargetHandle.GetCPUHandle(), cubeMapFilterInfo.FilterTarget->GetUnorderedAccessViewHandle(mipIndex, cubeSideIndex).GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            computeContext->SetDescriptorTable(2, filterTargetHandle.GetGPUHandle());
            computeContext->SetConstants(3, 12, &filterInfo);
            computeContext->Dispatch((uint32)filterInfo.sourceDimensions.X / 8, (uint32)filterInfo.sourceDimensions.Y / 8, 1);

            filterInfo.sourceDimensions = Vector2(filterInfo.sourceDimensions.X / 2, filterInfo.sourceDimensions.Y / 2);
        }
    }

    cubeMapFilterInfo.FilterTarget->SetComputeFence(computeContext->Flush(mDirect3DManager->GetContextManager()->GetQueueManager()));
}

void TextureManager::ProcessGeneration(GenerateTextureInfo &generationInfo, RenderPassDescriptorHeap *srvHeap)
{
    ComputeContext *computeContext = mDirect3DManager->GetContextManager()->GetComputeContext();
    computeContext->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap->GetHeap());

    computeContext->SetPipelineState(generationInfo.GenerationShader);
    computeContext->SetRootSignature(generationInfo.GenerationShader->GetRootSignature());

    DescriptorHeapHandle generateTargetHandle = srvHeap->GetHeapHandleBlock(1);
    mDirect3DManager->GetDevice()->CopyDescriptorsSimple(1, generateTargetHandle.GetCPUHandle(), generationInfo.GenerateTarget->GetUnorderedAccessViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    computeContext->SetDescriptorTable(0, generateTargetHandle.GetGPUHandle());
    computeContext->Dispatch(generationInfo.GenerateTarget->GetWidth() / 8, generationInfo.GenerateTarget->GetHeight() / 8, 1);

    generationInfo.GenerationFence = computeContext->Flush(mDirect3DManager->GetContextManager()->GetQueueManager());
}

FilteredCubeMapRenderTexture *TextureManager::FilterCubeMap(Texture *cubeMapToFilter, Sampler *envSampler, ShaderPSO *envFilterShader, uint32 dimensionSize, uint32 numMips)
{
    FilteredCubeMapRenderTexture *filterTarget = mDirect3DManager->GetContextManager()->CreateFilteredCubeMapRenderTexture(dimensionSize, DXGI_FORMAT_R8G8B8A8_UNORM, numMips);

    CubeMapFilterInfo filterInfo;
    filterInfo.CubeMapToFilter = cubeMapToFilter;
    filterInfo.DimensionSize = dimensionSize;
    filterInfo.NumMips = numMips;
    filterInfo.EnvironmentFilterShader = envFilterShader;
    filterInfo.EnvironmentSampler = envSampler;
    filterInfo.FilterTarget = filterTarget;
    mCubeMapFilters.Add(filterInfo);

    return filterTarget;
}

RenderTarget *TextureManager::GenerateTexture(ShaderPSO *generationShader, uint32 width, uint32 height)
{
    RenderTarget *generationTarget = mDirect3DManager->GetContextManager()->CreateRenderTarget(width, height, DXGI_FORMAT_R16G16_FLOAT, true, 1, 1, 0);
    generationTarget->SetIsReady(false); //render targets are defaulted to ready, but it isn't really ready for usage until it's filled, so set not ready for now
    mDirect3DManager->GetContextManager()->GetGraphicsContext()->TransitionResource(generationTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

    GenerateTextureInfo generateInfo;
    generateInfo.GenerationFence = 0;
    generateInfo.GenerateTarget = generationTarget;
    generateInfo.GenerationShader = generationShader;
    mGeneratedTextures.Add(generateInfo);

    return generationTarget;
}