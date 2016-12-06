/*
#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"

struct Direct3DUpload
{
	Direct3DUpload()
	{
		Reset();
	}

	void Reset()
	{
		UploadLocation = 0;
		UploadSize = 0;
		UploadPadding = 0;
		FenceValue = UINT64_MAX;
	}

	ID3D12CommandAllocator *CommandAllocator;
	uint64 UploadLocation;
	uint64 UploadSize;
	uint64 UploadPadding;
	uint64 FenceValue;
};

struct Direct3DUploadInfo
{
	ID3D12GraphicsCommandList* CommandList;
	void* CPUAddress = nullptr;
	uint64 ResourceOffset = 0;
	ID3D12Resource* Resource = nullptr;
};

class Direct3DUploadManager
{
public:
	Direct3DUploadManager(ID3D12Device *device, D3D12_HEAP_PROPERTIES &heapProperties);
	~Direct3DUploadManager();

	void ClearFinishedUploads(uint64 flushCount);
	bool AllocUploadSubmission(uint64 size);
	Direct3DUploadInfo GetUploadInfoForBuffer(uint64 size);
	void ResourceUploadEnd(Direct3DUploadInfo& context);

private:
	ID3D12Resource *mUploadBuffer;
	ID3D12CommandQueue *mCommandQueue;
	ID3D12GraphicsCommandList *mCommandList;

	ID3D12Fence *mFence;
	uint64 mFenceValue;
	HANDLE mFenceEvent;

	uint8* mBufferAddress;
	uint64 mBufferStart;
	uint64 mBufferUsed;

	Direct3DUpload mUploads[MAX_GPU_UPLOADS];
	uint64 mUploadSubmissionStart;
	uint64 mUploadSubmissionUsed;
};*/