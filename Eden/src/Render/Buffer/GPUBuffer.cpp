#include "Render/Buffer/GPUBuffer.h"

GPUBuffer::GPUBuffer(ID3D12Device *device, uint32 elementCount, uint32 elementSize, void* initData)
{
	mElementCount = elementCount;
	mElementSize = elementSize;
	mBufferSize = elementCount * elementSize;
	mUsageState = D3D12_RESOURCE_STATE_COMMON;

	Application::Assert(mBufferSize > 0);

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Alignment = 0;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Flags = mResourceFlags;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Width = (uint64)mBufferSize;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	Direct3DUtils::ThrowIfHRESULTFailed(device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, mUsageState,
		NULL, IID_PPV_ARGS(&mResource)));

	mGPUAddress = mResource->GetGPUVirtualAddress();

	if (initData)
	{
		CommandContext::InitializeBuffer(*this, initData, mBufferSize);
	}

	mResource->SetName(L"GPUBuffer::mResource");
}

GPUBuffer::~GPUBuffer()
{

}