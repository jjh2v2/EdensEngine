#include "Render/Texture/TextureManager.h"

TextureManager::TextureManager()
{

}

TextureManager::~TextureManager()
{
	mTextureLookup.clear();

	for (uint32 i = 0; i < mTextures.CurrentSize(); i++)
	{
		delete mTextures[i];
		mTextures[i] = NULL;
	}
	mTextures.Clear();
}

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
