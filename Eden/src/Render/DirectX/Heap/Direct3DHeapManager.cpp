#include "Render/DirectX/Heap/Direct3DHeapManager.h"
#include "Util/Math/MathHelper.h"

Direct3DHeapManager::Direct3DHeapManager(ID3D12Device* device)
{
    mDevice = device;

	mRTVDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTV_DESCRIPTOR_HEAP_SIZE, false);
	mSRVDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, SRV_DESCRIPTOR_HEAP_SIZE, false);
	mDSVDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DSV_DESCRIPTOR_HEAP_SIZE, false);
	mSamplerDescriptorHeap = new DynamicDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SAMPLER_DESCRIPTOR_HEAP_SIZE, false);
}

Direct3DHeapManager::~Direct3DHeapManager()
{
    //I don't personally like using auto but STL makes it impossible not to
    for (auto iter = mRenderPassDescriptorHeapGroups.begin(); iter != mRenderPassDescriptorHeapGroups.end(); iter++)
    {
        for (uint32 i = 0; i < FRAME_BUFFER_COUNT; i++)
        {
            if (iter->second.SRVHeap[i])
            {
                delete iter->second.SRVHeap[i];
            }

            if (iter->second.SamplerHeap[i])
            {
                delete iter->second.SamplerHeap[i];
            }
        }
    }

	delete mRTVDescriptorHeap;
	delete mSRVDescriptorHeap;
	delete mDSVDescriptorHeap;
	delete mSamplerDescriptorHeap;
}

RenderPassDescriptorHeap *Direct3DHeapManager::GetRenderPassDescriptorHeapFor(RenderPassDescriptorHeapType passType, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32 frameIndex, uint32 numDescriptors, bool reset)
{
    numDescriptors = MathHelper::AlignU32(numDescriptors, RENDER_PASS_DESCRIPTOR_HEAP_MULTIPLE); //just for the sake of leaving a bit of leg-room for growth

    if (mRenderPassDescriptorHeapGroups.find(passType) == mRenderPassDescriptorHeapGroups.end())
    {
        mRenderPassDescriptorHeapGroups.insert(std::pair<RenderPassDescriptorHeapType, RenderPassDescriptorHeapGroup>(passType, RenderPassDescriptorHeapGroup()));
    }

    RenderPassDescriptorHeapGroup &heapGroup = mRenderPassDescriptorHeapGroups[passType];
    RenderPassDescriptorHeap *heapToReturn = NULL;

    switch (heapType)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        if (!heapGroup.SRVHeap[frameIndex] || heapGroup.SRVHeap[frameIndex]->GetMaxDescriptors() < numDescriptors)
        {
            if (heapGroup.SRVHeap[frameIndex])
            {
                Application::Assert(reset == true); //can't nuke a descriptor heap unless we're resetting, because otherwise we're already using it
                delete heapGroup.SRVHeap[frameIndex];
            }

            heapGroup.SRVHeap[frameIndex] = new RenderPassDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, numDescriptors);
        }

        heapToReturn = heapGroup.SRVHeap[frameIndex];
        break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        if (!heapGroup.SamplerHeap[frameIndex] || heapGroup.SamplerHeap[frameIndex]->GetMaxDescriptors() < numDescriptors)
        {
            if (heapGroup.SamplerHeap[frameIndex])
            {
                Application::Assert(reset == true); //can't nuke a descriptor heap unless we're resetting, because otherwise we're already using it
                delete heapGroup.SamplerHeap[frameIndex];
            }

            heapGroup.SamplerHeap[frameIndex] = new RenderPassDescriptorHeap(mDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, numDescriptors);
        }

        heapToReturn = heapGroup.SamplerHeap[frameIndex];
        break;
    default:
        bool heapTypeNotSupported = false;
        Application::Assert(heapTypeNotSupported);
        break;
    }

    if (heapToReturn && reset)
    {
        heapToReturn->Reset();
    }

    return heapToReturn;
}