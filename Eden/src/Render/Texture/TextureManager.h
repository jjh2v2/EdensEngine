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
    TextureUploadState_Initialized,
    TextureUploadState_Read,
    TextureUploadState_Upload,
    TextureUploadState_Transition,
    TextureUploadState_Completed,
    TextureUploadState_Delete
};

struct TextureUpload
{
    TextureUpload();
    ~TextureUpload();

    TextureUploadState UploadState;
    Texture *TextureToUpload;
    WCHAR *FilePath;

    D3D12_RESOURCE_DESC TextureDesc;
    DirectX::ScratchImage *ImageData;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];

    Direct3DUploadInfo UploadInfo;
    uint64 UploadFence;

    Job *ReadJob;
};

class TextureManager
{
public:
	TextureManager(Direct3DManager *direct3DManager);
	~TextureManager();

    void ProcessCurrentUploads();
	void LoadTextureManifest();
	
	Texture *GetTexture(const std::string &textureName, bool async = true);
    static Texture* GetDefaultTexture() { return mDefaultTexture; }

private:
    Texture *LoadTexture(WCHAR *filePath, bool async = true);

    //These could be combined into one with a switch, but creating unique job types will make them easier to 
    //track when I add job type IDs and a tracker for testing and debugging
    class TextureReadJob : public Job
    {
    public:
        TextureReadJob(TextureUpload *textureUpload, TextureManager *textureManager);
        virtual void Execute();

    private:
        TextureManager *mTextureManager;
        TextureUpload *mTextureUpload;
    };

    class TextureBackgroundUpload : public UploadContext::BackgroundUpload
    {
    public:
        TextureBackgroundUpload(ID3D12Device *device, Direct3DQueueManager *queueManager, TextureUpload *textureUpload);
        virtual void ProcessUpload(UploadContext *uploadContext);

    private:
        ID3D12Device *mDevice;
        Direct3DQueueManager *mQueueManager;
        TextureUpload *mTextureUpload;
    };


    void ProcessFileRead(TextureUpload *currentUpload);
    void ProcessCopy(TextureUpload *currentUpload);
    void ProcessTransition(TextureUpload *currentUpload, uint64 currentFence);

	Direct3DManager *mDirect3DManager;

	std::map<std::string, TextureLookup> mTextureLookup;
	DynamicArray<Texture*> mTextures;
	ManifestLoader mManifestLoader;

    DynamicArray<TextureUpload*> mTextureUploads;

    std::mutex mTextureLookupMutex;
    std::mutex mTextureUploadMutex;

	static Texture *mDefaultTexture;
};