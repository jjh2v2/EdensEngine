#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/Direct3DManager.h"
#include "Core/Containers/DynamicArray.h"

struct Direct3DUpload
{
	Direct3DUpload()
	{
		UploadLocation = 0;
		UploadSize = 0;
		UploadPadding = 0;
		FenceValue = UINT64_MAX;
	}

	uint64 UploadLocation;
	uint64 UploadSize;
	uint64 UploadPadding;
	uint64 FenceValue;
};

class Direct3DUploadManager
{
public:
	Direct3DUploadManager(Direct3DManager *direct3DManager);
	~Direct3DUploadManager();

private:
	ID3D12Resource *mUploadBuffer;
	ID3D12CommandQueue *mCommandQueue;
	ID3D12GraphicsCommandList *mCommandList;
	ID3D12CommandAllocator *mCommandAllocator;

	ID3D12Fence *mFence;
	uint64 mFenceValue;
	HANDLE mFenceEvent;

	uint8* mBufferAddress;
	uint64 mBufferStart;
	uint64 mBufferUsed;

	DynamicArray<Direct3DUpload> mCurrentUploads;
};