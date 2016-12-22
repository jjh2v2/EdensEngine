#include "Render/Texture/TextureManager.h"
#include "Asset/Texture/DirectXTex.h"
#include "Util/String/StringConverter.h"

TextureManager::TextureManager(Direct3DManager *direct3DManager)
{
	mDirect3DManager = direct3DManager;
}

TextureManager::~TextureManager()
{
	for (uint32 i = 0; i < mTextures.CurrentSize(); i++)
	{
		mDirect3DManager->GetHeapManager()->FreeSRVDescriptorHeapHandle(mTextures[i]->GetDescriptorHeapHandle());
		mTextures[i]->GetTextureResource()->GetResource()->Release();	//TDA: put this somewhere else
		delete mTextures[i]->GetTextureResource();						//TDA: put this somewhere else
		mTextures[i]->SetTextureResource(NULL);
		delete mTextures[i];
	}

	mTextures.Clear();
}

void TextureManager::LoadAllTextures()
{
	//TDA: This isn't finished EDIT: Thanks past Alex, that's a really helpful comment.
	mManifestLoader.LoadManifest(ApplicationSpecification::TextureManifestFileLocation);

	DynamicArray<std::string> &fileNames = mManifestLoader.GetFileNames();
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

	{
		WCHAR *convertedString = StringConverter::StringToWCHARAlloc(mTextureLookup["MageDiffuseFire"].TextureFilePath);
		Texture *texture = LoadTexture(convertedString);
		mTextureLookup["MageDiffuseFire"].TextureRef = texture;
		mTextures.Add(texture);
		delete[] convertedString;
	}
	{
		WCHAR *convertedString = StringConverter::StringToWCHARAlloc(mTextureLookup["MageDiffuseIce"].TextureFilePath);
		Texture *texture = LoadTexture(convertedString);
		mTextureLookup["MageDiffuseIce"].TextureRef = texture;
		mTextures.Add(texture);
		delete[] convertedString;
	}
	
}

Texture *TextureManager::LoadTexture(WCHAR *filePath)
{
	Texture *newTexture = new Texture();
	DirectX::ScratchImage imageData;

	HRESULT loadResult = DirectX::LoadFromDDSFile(filePath, DirectX::DDS_FLAGS_NONE, nullptr, imageData);
	assert(loadResult == S_OK);

	const DirectX::TexMetadata& textureMetaData = imageData.GetMetadata();
	DXGI_FORMAT textureFormat = textureMetaData.format;
	bool is3DTexture = textureMetaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D;

	newTexture->SetDimensions(Vector3((float)textureMetaData.width, (float)textureMetaData.height, (float)textureMetaData.depth));
	newTexture->SetMipCount((uint32)textureMetaData.mipLevels);
	newTexture->SetArraySize((uint32)textureMetaData.arraySize);
	newTexture->SetFormat(textureMetaData.format);
	newTexture->SetIsCubeMap(textureMetaData.IsCubemap());

	if (ApplicationSpecification::ForceAllTexturesToSRGB)
	{
		textureFormat = DirectX::MakeSRGB(textureFormat);
	}

	D3D12_RESOURCE_DESC textureDesc;
	textureDesc.MipLevels = (uint16)textureMetaData.mipLevels;
	textureDesc.Format = textureFormat;
	textureDesc.Width = (uint32)textureMetaData.width;
	textureDesc.Height = (uint32)textureMetaData.height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = is3DTexture ? uint16(textureMetaData.depth) : uint16(textureMetaData.arraySize);
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = is3DTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Alignment = 0;
	
	D3D12_HEAP_PROPERTIES defaultProperties;
	defaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultProperties.CreationNodeMask = 0;
	defaultProperties.VisibleNodeMask = 0;
	
	ID3D12Resource *newTextureResource = NULL;
	mDirect3DManager->GetDevice()->CreateCommittedResource(&defaultProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&newTextureResource));

	TextureResource *textureGPUResource = new TextureResource(newTextureResource, D3D12_RESOURCE_STATE_COPY_DEST);
	newTexture->SetTextureResource(textureGPUResource);
	newTexture->SetDescriptorHeapHandle(mDirect3DManager->GetHeapManager()->GetNewSRVDescriptorHeapHandle());

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

	mDirect3DManager->GetDevice()->CreateShaderResourceView(newTextureResource, srvDescPointer, newTexture->GetDescriptorHeapHandle().GetCPUHandle());

	const uint64 numSubResources = textureMetaData.mipLevels * textureMetaData.arraySize;
	uint64 textureMemorySize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];
	UINT numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];
	UINT64 rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];

	mDirect3DManager->GetDevice()->GetCopyableFootprints(&textureDesc, 0, uint32(numSubResources), 0, layouts, numRows, rowSizesInBytes, &textureMemorySize);

	UploadContext *uploadContext = mDirect3DManager->GetContextManager()->GetUploadContext();
	Direct3DUploadInfo uploadInfo = uploadContext->BeginUpload(textureMemorySize, mDirect3DManager->GetContextManager()->GetQueueManager());
	uint8* uploadMem = reinterpret_cast<uint8*>(uploadInfo.CPUAddress);

	for (uint64 arrayIdx = 0; arrayIdx < textureMetaData.arraySize; ++arrayIdx)
	{
		for (uint64 mipIdx = 0; mipIdx < textureMetaData.mipLevels; ++mipIdx)
		{
			const uint64 subResourceIdx = mipIdx + (arrayIdx * textureMetaData.mipLevels);

			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = layouts[subResourceIdx];
			const uint64 subResourceHeight = numRows[subResourceIdx];
			const uint64 subResourcePitch = subResourceLayout.Footprint.RowPitch;
			const uint64 subResourceDepth = subResourceLayout.Footprint.Depth;
			uint8* dstSubResourceMem = reinterpret_cast<uint8*>(uploadMem) + subResourceLayout.Offset;

			for (uint64 z = 0; z < subResourceDepth; ++z)
			{
				const DirectX::Image* subImage = imageData.GetImage(mipIdx, arrayIdx, z);
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
		dst.pResource = newTextureResource;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = uint32(subResourceIdx);
		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = uploadInfo.Resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = layouts[subResourceIdx];
		src.PlacedFootprint.Offset += uploadInfo.ResourceOffset;
		uploadContext->CopyTextureRegion(&dst, &src);
	}

	uint64 uploadFenceValue = uploadContext->EndUpload(uploadInfo, mDirect3DManager->GetContextManager()->GetQueueManager());
	
	//TDA: stall here until loaded until async is implemented. //EndUpload currently stalls so it's okay for now

	mDirect3DManager->GetContextManager()->GetGraphicsContext()->TransitionResource((*textureGPUResource), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

	return newTexture;
}