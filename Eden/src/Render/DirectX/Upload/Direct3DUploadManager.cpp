#include "Render/DirectX/Upload/Direct3DUploadManager.h"
#include "Render/DirectX/D3D12Helper.h"

Direct3DUploadManager::Direct3DUploadManager(Direct3DManager *direct3DManager)
{
	ID3D12Device *device = direct3DManager->GetDevice();

	for (uint32 i = 0; i < MAX_TEXTURE_UPLOADS; i++)
	{
		//allocator can only be on one active command list at a time
		Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mUploads[i].CommandAllocator)));
	}

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mUploads[0].CommandAllocator, NULL, IID_PPV_ARGS(&mCommandList)));
	mCommandList->Close();

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCommandQueue)));

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	mFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	mFenceValue = 0;

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Width = uint32(UPLOAD_BUFFER_SIZE);
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadBufferDesc.Alignment = 0;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&direct3DManager->GetUploadHeapProperties(), D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadBuffer)));

	D3D12_RANGE readRange = {};
	Direct3DUtils::ThrowIfHRESULTFailed(mUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mBufferAddress)));

	mBufferStart = 0;
	mBufferUsed = 0;
	mUploadSubmissionStart = 0;
	mUploadSubmissionUsed = 0;
}

Direct3DUploadManager::~Direct3DUploadManager()
{

}


void Direct3DUploadManager::ClearFinishedUploads(uint64 flushCount)
{
	const uint64 start = mUploadSubmissionStart;
	const uint64 numUsed = mUploadSubmissionUsed;

	for (uint64 i = 0; i < numUsed; ++i)
	{
		Direct3DUpload &submission = mUploads[(start + i) % MAX_GPU_UPLOADS];

		if (i < flushCount)
		{
			if (mFence->GetCompletedValue() < submission.FenceValue)
			{
				Direct3DUtils::ThrowIfHRESULTFailed(mFence->SetEventOnCompletion(submission.FenceValue, mFenceEvent));
				WaitForSingleObject(mFenceEvent, INFINITE);
			}
		}

		if (mFence->GetCompletedValue() >= submission.FenceValue)
		{
			mUploadSubmissionStart = (mUploadSubmissionStart + 1) % MAX_GPU_UPLOADS;
			mUploadSubmissionUsed -= 1;
			mBufferStart = (mBufferStart + submission.UploadPadding) % UPLOAD_BUFFER_SIZE;
			Application::Assert(submission.UploadLocation == mBufferStart);
			Application::Assert(mBufferStart + submission.UploadSize <= UPLOAD_BUFFER_SIZE);
			mBufferStart = (mBufferStart + submission.UploadSize) % UPLOAD_BUFFER_SIZE;
			mBufferUsed -= (submission.UploadSize + submission.UploadPadding);
			submission.Reset();

			if (mBufferUsed == 0)
			{
				mBufferStart = 0;
			}
		}
	}
}

bool Direct3DUploadManager::AllocUploadSubmission(uint64 size)
{
	Application::Assert(mUploadSubmissionUsed <= MAX_GPU_UPLOADS);
	if (mUploadSubmissionUsed == MAX_GPU_UPLOADS)
	{
		return false;
	}

	const uint64 newSubmissionId = (mUploadSubmissionStart + mUploadSubmissionUsed) % MAX_GPU_UPLOADS;
	Application::Assert(mUploads[newSubmissionId].UploadSize == 0);
	Application::Assert(mBufferUsed <= UPLOAD_BUFFER_SIZE);

	if (size > (UPLOAD_BUFFER_SIZE - mBufferUsed))
	{
		return false;
	}

	const uint64 start = mBufferStart;
	const uint64 end = mBufferStart + mBufferUsed;
	uint64 allocOffset = UINT64_MAX;
	uint64 padding = 0;
	if (end < UPLOAD_BUFFER_SIZE)
	{
		const uint64 endAmt = UPLOAD_BUFFER_SIZE - end;
		if (endAmt >= size)
		{
			allocOffset = end;
		}
		else if (start >= size)
		{
			// Wrap around to the beginning
			allocOffset = 0;
			mBufferUsed += endAmt;
			padding = endAmt;
		}
	}
	else
	{
		const uint64 wrappedEnd = end % UPLOAD_BUFFER_SIZE;
		Application::Assert(start > wrappedEnd);
		if ((start - wrappedEnd) >= size)
		{
			allocOffset = wrappedEnd;
		}
	}

	if (allocOffset == UINT64_MAX)
	{
		return false;
	}

	mUploadSubmissionUsed += 1;
	mBufferUsed += size;

	++mFenceValue;

	mUploads[newSubmissionId].UploadLocation = allocOffset;
	mUploads[newSubmissionId].UploadSize = size;
	mUploads[newSubmissionId].FenceValue = mFenceValue;
	mUploads[newSubmissionId].UploadPadding = padding;

	return true;
}

Direct3DUploadInfo Direct3DUploadManager::GetUploadInfoForBuffer(uint64 size)
{
	size = Application::Align(size, 512);
	Application::Assert(size <= UPLOAD_BUFFER_SIZE);

	ClearFinishedUploads(0);
	while (AllocUploadSubmission(size) == false)
	{
		ClearFinishedUploads(1);
	}

	//Assert_(UploadSubmissionUsed > 0);
	const uint64 submissionIdx = (mUploadSubmissionStart + (mUploadSubmissionUsed - 1)) % MAX_GPU_UPLOADS;
	Direct3DUpload &submission = mUploads[submissionIdx];
	//Assert_(submission.Size == size);

	Application::Assert(submission.CommandAllocator->Reset());
	Application::Assert(mCommandList->Reset(submission.CommandAllocator, NULL));

	Direct3DUploadInfo uploadInfo;
	uploadInfo.CommandList = mCommandList;
	uploadInfo.Resource = mUploadBuffer;
	uploadInfo.CPUAddress = mBufferAddress + submission.UploadLocation;
	uploadInfo.ResourceOffset = submission.UploadLocation;

	return uploadInfo;
}

void Direct3DUploadManager::ResourceUploadEnd(Direct3DUploadInfo& context)
{
	// Finish off and execute the command list
	Application::Assert(mCommandList->Close());
	ID3D12CommandList* cmdLists[1] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(1, cmdLists);

	Application::Assert(mUploadSubmissionUsed > 0);
	const uint64 submissionIdx = (mUploadSubmissionStart + (mUploadSubmissionUsed - 1)) % MAX_GPU_UPLOADS;
	Direct3DUpload &submission = mUploads[submissionIdx];
	Application::Assert(submission.UploadSize != 0);

	mCommandQueue->Signal(mFence, submission.FenceValue);
}