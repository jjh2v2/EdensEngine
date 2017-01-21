#pragma once
#include "Render/Texture/Texture.h"
#include <map>
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/DirectX/Direct3DManager.h"

struct TextureLookup
{
	std::string TextureFilePath;
	Texture *TextureRef;
};

class TextureManager
{
public:
	TextureManager(Direct3DManager *direct3DManager);
	~TextureManager();

	void LoadAllTextures();
	Texture *LoadTexture(WCHAR *filePath);
	Texture *GetTexture(std::string textureName) { return mTextureLookup[textureName].TextureRef; }

private:
	Direct3DManager *mDirect3DManager;

	std::map<std::string, TextureLookup> mTextureLookup;
	DynamicArray<Texture*> mTextures;
	ManifestLoader mManifestLoader;

	Texture *mWhiteTexture; //TDA: Actually use this
};