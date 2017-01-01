#include "Render/DirectX/Heap/Direct3DHeapManager.h"

Direct3DHeapManager::Direct3DHeapManager(ID3D12Device* device)
{
	mRTVDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTV_DESCRIPTOR_HEAP_SIZE, false);
	mSRVDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, SRV_DESCRIPTOR_HEAP_SIZE, false);
	mDSVDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DSV_DESCRIPTOR_HEAP_SIZE, false);
	mSamplerDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SAMPLER_DESCRIPTOR_HEAP_SIZE, false);
}

Direct3DHeapManager::~Direct3DHeapManager()
{
	delete mRTVDescriptorHeap;
	delete mSRVDescriptorHeap;
	delete mDSVDescriptorHeap;
	delete mSamplerDescriptorHeap;
}