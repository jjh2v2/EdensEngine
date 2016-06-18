#include "Render/DirectX/Upload/Direct3DUploadManager.h"
#include "Render/DirectX/D3D12Helper.h"

Direct3DUploadManager::Direct3DUploadManager(Direct3DManager *direct3DManager)
{
	ID3D12Device *device = direct3DManager->GetDevice();

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCommandAllocator)));
	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCommandAllocator, NULL, IID_PPV_ARGS(&mCommandList)));
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
	uploadBufferDesc.Width = uint32(UPLOAD_TEXTURE_BUFFER_SIZE);
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
}

Direct3DUploadManager::~Direct3DUploadManager()
{

}