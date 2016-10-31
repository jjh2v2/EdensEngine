#include "Render/DirectX/Queue/Direct3DQueue.h"
#include "Util/Math/MathHelper.h"

Direct3DQueue::Direct3DQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandType)
{
	mQueueType = commandType;
	mCommandQueue = NULL;
	mFence = NULL;
	mNextFenceValue = ((uint64_t)commandType << 56 | 1);
	mLastCompletedFenceValue = ((uint64_t)commandType << 56);

	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = mQueueType;
	QueueDesc.NodeMask = 1;
	device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&mCommandQueue));
	mCommandQueue->SetName(L"Direct3DQueue::mCommandQueue");

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	mFence->SetName(L"Direct3DQueue::mFence");
	mFence->Signal((uint64_t)mQueueType << 56);

	mFenceEventHandle = CreateEvent(nullptr, false, false, nullptr);

	Application::Assert(mFenceEventHandle != INVALID_HANDLE_VALUE);
}

Direct3DQueue::~Direct3DQueue()
{
	if (mCommandQueue == NULL)
	{
		return;
	}

	CloseHandle(mFenceEventHandle);

	mFence->Release();
	mFence = NULL;

	mCommandQueue->Release();
	mCommandQueue = NULL;
}

uint64 Direct3DQueue::IncrementFence()
{
	std::lock_guard<std::mutex> lockGuard(mFenceMutex);
	mCommandQueue->Signal(mFence, mNextFenceValue);
	return mNextFenceValue++;
}

bool Direct3DQueue::IsFenceComplete(uint64 fenceValue)
{
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (fenceValue > mLastCompletedFenceValue)
	{
		mLastCompletedFenceValue = MathHelper::Max(mLastCompletedFenceValue, mFence->GetCompletedValue());
	}

	return fenceValue <= mLastCompletedFenceValue;
}

void Direct3DQueue::StallForFence(Direct3DQueue* otherQueue, uint64 fenceValue)
{
	mCommandQueue->Wait(otherQueue->GetFence(), fenceValue);
}

void Direct3DQueue::StallForQueue(Direct3DQueue* otherQueue)
{
	mCommandQueue->Wait(otherQueue->GetFence(), otherQueue->GetNextFenceValue() - 1);
}

void Direct3DQueue::WaitForFence(uint64 fenceValue)
{
	if (IsFenceComplete(fenceValue))
	{
		return;
	}

	// TDA: Address the below, probably make some event map based on order of fence values
	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		std::lock_guard<std::mutex> lockGuard(mEventMutex);

		mFence->SetEventOnCompletion(fenceValue, mFenceEventHandle);
		WaitForSingleObject(mFenceEventHandle, INFINITE);
		mLastCompletedFenceValue = fenceValue;
	}
}

uint64 Direct3DQueue::ExecuteCommandList(ID3D12CommandList* commandList)
{
	std::lock_guard<std::mutex> lockGuard(mFenceMutex);

	Direct3DUtils::ThrowIfHRESULTFailed(((ID3D12GraphicsCommandList*)commandList)->Close());
	mCommandQueue->ExecuteCommandLists(1, &commandList);
	mCommandQueue->Signal(mFence, mNextFenceValue);

	return mNextFenceValue++;
}