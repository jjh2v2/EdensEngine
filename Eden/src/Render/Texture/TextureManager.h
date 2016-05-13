#pragma once
#include "Render/Texture/Texture.h"
#include <map>
#include "Asset/Manifest/ManifestLoader.h"
#include "Asset/DDS/DDSLoader.h"
#include "Util/String/StringConverter.h"
#include "Core/Containers/DynamicArray.h"

struct TextureLookup
{
	std::string TextureFilePath;
	Texture *TextureRef;
};

class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	void LoadAllTextures(ID3D11Device* device, ID3D11DeviceContext *context, ID3DX11ThreadPump *mThreadPump);
	Texture *GetTexture(std::string textureName, bool loadAsync = true);

private:
	Texture *LoadTexture(ID3D11Device* device, WCHAR *fileName, bool loadAsync = true);
	Texture *LoadTexture(ID3D11Device* device, WCHAR *fileName, D3DX11_IMAGE_LOAD_INFO *info);

	ID3D11Device *mDevice;									
	ID3DX11ThreadPump *mThreadPump;
	std::map<std::string, TextureLookup> mTextureLookup;
	DynamicArray<Texture*> mTextures;
	ManifestLoader mManifestLoader;
	DDSLoader mDDSLoader;

	Texture *mWhiteTexture;
};