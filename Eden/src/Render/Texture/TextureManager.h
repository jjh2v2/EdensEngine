#pragma once
#include "Render/Texture/Texture.h"
#include "Asset/Manifest/ManifestLoader.h"
#include "Render/DirectX/Direct3DManager.h"
#include "Asset/Texture/DirectXTex.h"
#include "Core/Threading/Job.h"
#include <map>

struct TextureLookup
{
	std::string TextureFilePath;
	Texture *TextureRef;
};

enum TextureUploadState
{
    TextureUploadState_Pending,
    TextureUploadState_Read,
    TextureUploadState_Copy,
    TextureUploadState_Upload,
    TextureUploadState_Transition,
    TextureUploadState_Completed
};

struct TextureUpload
{
    TextureUploadState UploadState;
    Texture *TextureToUpload;
    WCHAR *FilePath;

    D3D12_RESOURCE_DESC TextureDesc;
    DirectX::ScratchImage *ImageData;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];

    Direct3DUploadInfo UploadInfo;
    uint64 UploadFence;
};

class TextureManager
{
public:
	TextureManager(Direct3DManager *direct3DManager);
	~TextureManager();

    void ProcessUploads();
	void LoadAllTextures();
	Texture *LoadTexture(WCHAR *filePath, bool async = false);
	Texture *GetTexture(const std::string &textureName, bool loadImmediate = false);

    static Texture* GetDefaultTexture() { return mDefaultTexture; }

private:

    void ProcessFileRead(TextureUpload &currentUpload);
    void ProcessCopy(TextureUpload &currentUpload);
    void ProcessUpload(TextureUpload &currentUpload, bool forceWait = false);
    void ProcessTransition(TextureUpload &currentUpload, uint64 currentFence);

	Direct3DManager *mDirect3DManager;

	std::map<std::string, TextureLookup> mTextureLookup;
	DynamicArray<Texture*> mTextures;
	ManifestLoader mManifestLoader;

    DynamicArray<TextureUpload> mTextureUploads;

	static Texture *mDefaultTexture;
};