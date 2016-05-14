#include "Render/Texture/TextureManager.h"
#include "Asset/Texture/DirectXTex.h"

TextureManager::TextureManager(ID3D12Device *device)
{
	mDevice = device;
	//DirectX::ScratchImage image;
	//const WCHAR* filePath = L"../Eden/data/Textures/Character/Mage/MageDiffuseFire.dds";

	//const std::wstring extension = GetFileExtension(filePath);
	//if (extension == L"DDS" || extension == L"dds")
	//{
		//HRESULT result = DirectX::LoadFromDDSFile(filePath, DirectX::DDS_FLAGS_NONE, nullptr, image);
	//}
}

TextureManager::~TextureManager()
{
	
}

void TextureManager::LoadAllTextures()
{
	mManifestLoader.LoadManifest(TextureManifestFileLocation);

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

	newTexture->SetDimensions(Vector3(textureMetaData.width, textureMetaData.height, textureMetaData.depth));
	newTexture->SetMipCount(textureMetaData.mipLevels);
	newTexture->SetArraySize(textureMetaData.arraySize);
	newTexture->SetFormat(textureMetaData.format);
	newTexture->SetIsCubeMap(textureMetaData.IsCubemap());

	if (ForceAllTexturesToSRGB)
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
	
	mDevice->CreateCommittedResource(DX12::GetDefaultHeapProps(), D3D12_HEAP_FLAG_NONE, &textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture.Resource)));

	return newTexture;
}

/*
Texture *TextureManager::GetTexture(std::string textureName, bool loadAsync)
{
	if(mTextureLookup[textureName].TextureRef == NULL)
	{
		//Only load textures up when we need them 
		WCHAR *convertedString = StringConverter::StringToWCHAR(mTextureLookup[textureName].TextureFilePath);
		Texture *texture = LoadTexture(mDevice, convertedString, loadAsync);
		delete [] convertedString;

		mTextureLookup[textureName].TextureRef = texture;
		mTextures.Add(texture);
		return texture;
	}

	return mTextureLookup[textureName].TextureRef;
}

void TextureManager::LoadAllTextures(ID3D11Device* device, ID3D11DeviceContext *context, ID3DX11ThreadPump *threadPump)
{
	DirectX::ScratchImage image;

	const std::wstring extension = GetFileExtension(filePath);
	if (extension == L"DDS" || extension == L"dds")
	{
		DXCall(DirectX::LoadFromDDSFile(filePath, DirectX::DDS_FLAGS_NONE, nullptr, image));
	}

	mDevice = device;
	mThreadPump = threadPump;
	mManifestLoader.LoadManifest("../../Eden/data/Manifests/TextureManifest.emf");

	DynamicArray<std::string> &fileNames = mManifestLoader.GetFileNames();
	for(uint32 i = 0; i < fileNames.CurrentSize(); i++)
	{
		size_t lastSlash = fileNames[i].find_last_of("/");
		size_t lastDot = fileNames[i].find_last_of(".");
		std::string justFileName = fileNames[i].substr(lastSlash+1, (lastDot-lastSlash)-1);

		TextureLookup textureLookup;
		textureLookup.TextureFilePath = fileNames[i];
		textureLookup.TextureRef = NULL;
		mTextureLookup.insert(std::pair<std::string, TextureLookup>(justFileName, textureLookup));
	}

	mWhiteTexture = GetTexture("default", false);
}

Texture *TextureManager::LoadTexture(ID3D11Device* device, WCHAR *fileName, bool loadAsync)
{
	Texture *texture = new Texture();
	if (loadAsync)
	{
		texture->Initialize(device, fileName, mThreadPump, mWhiteTexture->GetTexture());
	}
	else
	{
		texture->Initialize(device, fileName);
	}
	
	return texture;
}

Texture *TextureManager::LoadTexture(ID3D11Device* device, WCHAR *fileName, D3DX11_IMAGE_LOAD_INFO *info)
{
	Texture *texture = new Texture();
	texture->Initialize(device, fileName, info);
	return texture;
}
*/