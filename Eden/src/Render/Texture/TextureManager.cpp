#include "Render/Texture/TextureManager.h"
#include "Util/String/StringConverter.h"

Texture *TextureManager::mDefaultTexture = NULL;

TextureManager::TextureManager(Direct3DManager *direct3DManager)
{
	mDirect3DManager = direct3DManager;
}

TextureManager::~TextureManager()
{
    //check the pending uploads for any unreleased memory
    for (uint32 i = 0; i < mTextureUploads.CurrentSize(); i++)
    {
        if (mTextureUploads[i].FilePath)
        {
            delete[] mTextureUploads[i].FilePath;
        }

        if (mTextureUploads[i].ImageData)
        {
            delete mTextureUploads[i].ImageData;
        }
    }

    mTextureUploads.Clear();

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

void TextureManager::LoadAllTextures()
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

    mDefaultTexture = GetTexture("Default", true);
}

Texture *TextureManager::GetTexture(const std::string &textureName, bool loadImmediate)
{
	Texture *texture = mTextureLookup[textureName].TextureRef;

	if (!texture)
	{
		WCHAR *convertedString = StringConverter::StringToWCHARAlloc(mTextureLookup[textureName].TextureFilePath);
		texture = LoadTexture(convertedString, !loadImmediate);
		mTextureLookup[textureName].TextureRef = texture;
        mTextures.Add(texture);
	}

	return texture;
}

Texture *TextureManager::LoadTexture(WCHAR *filePath, bool async)
{
    TextureUpload newTextureUpload;
    newTextureUpload.TextureToUpload = new Texture();
    newTextureUpload.UploadState = TextureUploadState_Pending;
    newTextureUpload.FilePath = filePath;
    newTextureUpload.ImageData = new DirectX::ScratchImage();

    if (async)
    {
        mTextureUploads.Add(newTextureUpload);
    }
    else
    {
        ProcessFileRead(newTextureUpload);
        ProcessCopy(newTextureUpload);
        ProcessUpload(newTextureUpload, true);
        ProcessTransition(newTextureUpload, 0); //fake fence of 0 because we know it's uploaded
        newTextureUpload.TextureToUpload->SetIsReady(true);
    }

	return newTextureUpload.TextureToUpload;
}

void TextureManager::ProcessUploads()
{
    uint64 currentUploadFence = mDirect3DManager->GetContextManager()->GetQueueManager()->GetCopyQueue()->PollCurrentFenceValue();

    uint32 numUploads = mTextureUploads.CurrentSize();
    for (uint32 i = 0; i < numUploads; i++)
    {
        TextureUpload &currentUpload = mTextureUploads[i];
        switch (currentUpload.UploadState)
        {
        case TextureUploadState_Pending:
            currentUpload.UploadState = TextureUploadState_Read;
            break;
        case TextureUploadState_Read:
            ProcessFileRead(currentUpload);
            break;
        case TextureUploadState_Copy:
            ProcessCopy(currentUpload);
            break;
        case TextureUploadState_Upload:
            if (mDirect3DManager->GetContextManager()->GetUploadContext()->GetNumUploadsAvailable() > 0)
            {
                ProcessUpload(currentUpload);
            }
            break;
        case TextureUploadState_Transition:
            ProcessTransition(currentUpload, currentUploadFence);
            break;
        case TextureUploadState_Completed:
            currentUpload.TextureToUpload->SetIsReady(true);
            break;
        default:
            Application::Assert(false);
            break;
        }
    }
}


void TextureManager::ProcessFileRead(TextureUpload &currentUpload)
{
    HRESULT loadResult = DirectX::LoadFromDDSFile(currentUpload.FilePath, DirectX::DDS_FLAGS_NONE, nullptr, *currentUpload.ImageData);
    assert(loadResult == S_OK);

    const DirectX::TexMetadata& textureMetaData = currentUpload.ImageData->GetMetadata();
    DXGI_FORMAT textureFormat = textureMetaData.format;
    bool is3DTexture = textureMetaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D;

    currentUpload.TextureToUpload->SetDimensions(Vector3((float)textureMetaData.width, (float)textureMetaData.height, (float)textureMetaData.depth));
    currentUpload.TextureToUpload->SetMipCount((uint32)textureMetaData.mipLevels);
    currentUpload.TextureToUpload->SetArraySize((uint32)textureMetaData.arraySize);
    currentUpload.TextureToUpload->SetFormat(textureMetaData.format);
    currentUpload.TextureToUpload->SetIsCubeMap(textureMetaData.IsCubemap());

    if (ApplicationSpecification::ForceAllTexturesToSRGB)
    {
        textureFormat = DirectX::MakeSRGB(textureFormat);
    }

    currentUpload.TextureDesc.MipLevels = (uint16)textureMetaData.mipLevels;
    currentUpload.TextureDesc.Format = textureFormat;
    currentUpload.TextureDesc.Width = (uint32)textureMetaData.width;
    currentUpload.TextureDesc.Height = (uint32)textureMetaData.height;
    currentUpload.TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    currentUpload.TextureDesc.DepthOrArraySize = is3DTexture ? uint16(textureMetaData.depth) : uint16(textureMetaData.arraySize);
    currentUpload.TextureDesc.SampleDesc.Count = 1;
    currentUpload.TextureDesc.SampleDesc.Quality = 0;
    currentUpload.TextureDesc.Dimension = is3DTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    currentUpload.TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    currentUpload.TextureDesc.Alignment = 0;

    D3D12_HEAP_PROPERTIES defaultProperties;
    defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    defaultProperties.CreationNodeMask = 0;
    defaultProperties.VisibleNodeMask = 0;

    ID3D12Resource *newTextureResource = NULL;
    mDirect3DManager->GetDevice()->CreateCommittedResource(&defaultProperties, D3D12_HEAP_FLAG_NONE, &currentUpload.TextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&newTextureResource));

    TextureResource *textureGPUResource = new TextureResource(newTextureResource, D3D12_RESOURCE_STATE_COPY_DEST, mDirect3DManager->GetContextManager()->GetHeapManager()->GetNewSRVDescriptorHeapHandle());
    currentUpload.TextureToUpload->SetTextureResource(textureGPUResource);

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

    mDirect3DManager->GetDevice()->CreateShaderResourceView(newTextureResource, srvDescPointer, currentUpload.TextureToUpload->GetTextureResource()->GetShaderResourceViewHandle().GetCPUHandle());

    delete[] currentUpload.FilePath;
    currentUpload.FilePath = NULL;

    currentUpload.UploadState = TextureUploadState_Copy;
}

void TextureManager::ProcessCopy(TextureUpload &currentUpload)
{
    const DirectX::TexMetadata& textureMetaData = currentUpload.ImageData->GetMetadata();
    const uint64 numSubResources = textureMetaData.mipLevels * textureMetaData.arraySize;
    uint64 textureMemorySize = 0;
    UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
    UINT64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];

    mDirect3DManager->GetDevice()->GetCopyableFootprints(&currentUpload.TextureDesc, 0, uint32(numSubResources), 0, currentUpload.Layouts, numRows, rowSizesInBytes, &textureMemorySize);

    UploadContext *uploadContext = mDirect3DManager->GetContextManager()->GetUploadContext();
    currentUpload.UploadInfo = uploadContext->BeginUpload(textureMemorySize, mDirect3DManager->GetContextManager()->GetQueueManager());
    uint8* uploadMem = reinterpret_cast<uint8*>(currentUpload.UploadInfo.CPUAddress);

    for (uint64 arrayIdx = 0; arrayIdx < textureMetaData.arraySize; ++arrayIdx)
    {
        for (uint64 mipIdx = 0; mipIdx < textureMetaData.mipLevels; ++mipIdx)
        {
            const uint64 subResourceIdx = mipIdx + (arrayIdx * textureMetaData.mipLevels);

            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = currentUpload.Layouts[subResourceIdx];
            const uint64 subResourceHeight = numRows[subResourceIdx];
            const uint64 subResourcePitch = subResourceLayout.Footprint.RowPitch;
            const uint64 subResourceDepth = subResourceLayout.Footprint.Depth;
            uint8* dstSubResourceMem = reinterpret_cast<uint8*>(uploadMem) + subResourceLayout.Offset;

            for (uint64 z = 0; z < subResourceDepth; ++z)
            {
                const DirectX::Image* subImage = currentUpload.ImageData->GetImage(mipIdx, arrayIdx, z);
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

    currentUpload.UploadState = TextureUploadState_Upload;
}

void TextureManager::ProcessUpload(TextureUpload &currentUpload, bool forceWait)
{
    UploadContext *uploadContext = mDirect3DManager->GetContextManager()->GetUploadContext();
    const DirectX::TexMetadata& textureMetaData = currentUpload.ImageData->GetMetadata();
    const uint64 numSubResources = textureMetaData.mipLevels * textureMetaData.arraySize;

    for (uint64 subResourceIdx = 0; subResourceIdx < numSubResources; ++subResourceIdx)
    {
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = currentUpload.TextureToUpload->GetTextureResource()->GetResource();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = uint32(subResourceIdx);
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = currentUpload.UploadInfo.Resource;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = currentUpload.Layouts[subResourceIdx];
        src.PlacedFootprint.Offset += currentUpload.UploadInfo.ResourceOffset;
        uploadContext->CopyTextureRegion(&dst, &src);
    }

    currentUpload.UploadFence = uploadContext->FlushUpload(currentUpload.UploadInfo, mDirect3DManager->GetContextManager()->GetQueueManager(), forceWait);

    currentUpload.UploadState = TextureUploadState_Transition;
}

void TextureManager::ProcessTransition(TextureUpload &currentUpload, uint64 currentFence)
{
    if (currentUpload.UploadFence <= currentFence)
    {
        mDirect3DManager->GetContextManager()->GetGraphicsContext()->TransitionResource((*currentUpload.TextureToUpload->GetTextureResource()), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
       
        delete currentUpload.ImageData;
        currentUpload.ImageData = NULL;

        currentUpload.UploadState = TextureUploadState_Completed;
    }
}