#include "Render/Buffer/GPUResource.h"

GPUResource::GPUResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState)
{
	mResource = resource;
	mUsageState = usageState;
	//MSDN: GetGPUVirtualAddress is only used for buffer resources, it will return zero for all texture resources. (and also throw errors)
	mGPUAddress = 0; //other buffer types set this themselves
    mIsReady = false;
}

GPUResource::~GPUResource()						//Sometimes we assign the resource from somewhere else, and don't want it released. How do we handle it? Current answer: we release it here, everything that creates a resource should do it through this
{
	mResource->Release();
	mResource = NULL;
}

TextureResource::TextureResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle shaderResourceViewHandle)
	:GPUResource(resource, usageState)
{
	mShaderResourceViewHandle = shaderResourceViewHandle;
}

TextureResource::~TextureResource()
{

}

BackBufferTarget::BackBufferTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, DescriptorHeapHandle renderTargetViewHandle)
	:GPUResource(resource, usageState)
{
	mRenderTargetViewHandle = renderTargetViewHandle;
}

BackBufferTarget::~BackBufferTarget()
{

}

RenderTarget::RenderTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC renderTargetDesc, DescriptorHeapHandle renderTargetViewHandle,
	DynamicArray<DescriptorHeapHandle> &renderTargetViewArray, DescriptorHeapHandle shaderResourceViewHandle, const UAVHandle &unorderedAccessView)
	:GPUResource(resource, usageState)
{
	mRenderTargetDesc = renderTargetDesc;
	mRenderTargetViewHandle = renderTargetViewHandle;
	mShaderResourceViewHandle = shaderResourceViewHandle;
    mUnorderedAccessView.HasUAV = unorderedAccessView.HasUAV;
    mUnorderedAccessView.BaseHandle = unorderedAccessView.BaseHandle;

    for (uint32 i = 0; i < unorderedAccessView.Handles.CurrentSize(); i++)
    {
        mUnorderedAccessView.Handles.Add(unorderedAccessView.Handles[i]);
    }

	for (uint32 i = 0; i < renderTargetViewArray.CurrentSize(); i++)
	{
		mRenderTargetViewArray.Add(renderTargetViewArray[i]);
	}

    mIsReady = true;
}

RenderTarget::~RenderTarget()
{

}

DescriptorHeapHandle RenderTarget::GetUnorderedAccessViewHandle(uint32 mipIndex, uint32 arrayIndex)
{
    Application::Assert(mUnorderedAccessView.HasUAV);
    Application::Assert(mRenderTargetDesc.MipLevels > mipIndex);
    Application::Assert(mRenderTargetDesc.DepthOrArraySize > arrayIndex);

    return mUnorderedAccessView.Handles[arrayIndex * mRenderTargetDesc.MipLevels + mipIndex];
}


DepthStencilTarget::DepthStencilTarget(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_RESOURCE_DESC depthStencilDesc, DescriptorHeapHandle depthStencilViewHandle, DescriptorHeapHandle readOnlyDepthStencilViewHandle,
	DynamicArray<DescriptorHeapHandle> &depthStencilViewArray, DynamicArray<DescriptorHeapHandle> &readOnlyDepthStencilViewArray, DescriptorHeapHandle shaderResourceViewHandle)
	:GPUResource(resource, usageState)
{
	mDepthStencilDesc = depthStencilDesc;
	mDepthStencilViewHandle = depthStencilViewHandle;
	mReadOnlyDepthStencilViewHandle = readOnlyDepthStencilViewHandle;
	mShaderResourceViewHandle = shaderResourceViewHandle;
    mIsReady = true;

	for (uint32 i = 0; i < depthStencilViewArray.CurrentSize(); i++)
	{
		mDepthStencilViewArray.Add(depthStencilViewArray[i]);
	}

	for (uint32 i = 0; i < readOnlyDepthStencilViewArray.CurrentSize(); i++)
	{
		mReadOnlyDepthStencilViewArray.Add(readOnlyDepthStencilViewArray[i]);
	}
}

DepthStencilTarget::~DepthStencilTarget()
{

}

VertexBuffer::VertexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 vertexStride, uint32 bufferSize)
	:GPUResource(resource, usageState)
{
	mGPUAddress = resource->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = vertexStride;
	mVertexBufferView.SizeInBytes = bufferSize;
	mVertexBufferView.BufferLocation = mGPUAddress;
}

VertexBuffer::~VertexBuffer()
{

}

IndexBuffer::IndexBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 bufferSize)
	:GPUResource(resource, usageState)
{
	mGPUAddress = resource->GetGPUVirtualAddress();
	mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	mIndexBufferView.SizeInBytes = bufferSize;
	mIndexBufferView.BufferLocation = mGPUAddress;
}

IndexBuffer::~IndexBuffer()
{

}

ConstantBuffer::ConstantBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferDesc, DescriptorHeapHandle constantBufferViewHandle)
	:GPUResource(resource, usageState)
{
	mGPUAddress = constantBufferDesc.BufferLocation;
	mConstantBufferViewDesc = constantBufferDesc;
	mConstantBufferViewHandle = constantBufferViewHandle;

	mMappedBuffer = NULL;
	mResource->Map(0, NULL, &mMappedBuffer);
}

ConstantBuffer::~ConstantBuffer()
{
	mResource->Unmap(0, NULL);
}

void ConstantBuffer::SetConstantBufferData(const void* bufferData, uint32 bufferSize)
{
	Application::Assert(bufferSize <= mConstantBufferViewDesc.SizeInBytes);
	memcpy(mMappedBuffer, bufferData, bufferSize);
}

StructuredBuffer::StructuredBuffer(ID3D12Resource* resource, ID3D12Resource *uploadResource, D3D12_RESOURCE_STATES usageState, bool isRaw, StructuredBufferAccess accessType, D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, DescriptorHeapHandle uavHandle, DescriptorHeapHandle srvHandle)
    :GPUResource(resource, usageState)
{
    mMappedBuffer = NULL;
    mUploadResource = uploadResource;
    mIsRaw = isRaw;
    mAccessType = accessType;
    mUAVDesc = uavDesc;
    mUnorderedAccessViewHandle = uavHandle;
    mShaderResourceViewHandle = srvHandle;
    
    if (mAccessType == CPU_WRITE_GPU_READ)
    {
        Direct3DUtils::ThrowIfHRESULTFailed(mResource->Map(0, NULL, &mMappedBuffer));
    }
}

StructuredBuffer::~StructuredBuffer()
{
    if (mAccessType == CPU_WRITE_GPU_READ)
    {
        mResource->Unmap(0, NULL);
    }

    if (mUploadResource)
    {
        mUploadResource->Release();
        mUploadResource = NULL;
    }
}

bool StructuredBuffer::CopyToBuffer(const void* bufferData, uint32 bufferSize)
{
    bool needsGPUCopy = false;

    switch (mAccessType)
    {
    case GPU_READ_WRITE:
        Application::Assert(false); //Trying to copy to a buffer that was designated as never written to by the CPU
        break;
    case CPU_WRITE_GPU_READ:
        //copy to the always-mapped resource
        Application::Assert(mMappedBuffer != NULL);
        Application::Assert(bufferSize <= (mUAVDesc.Buffer.NumElements * mUAVDesc.Buffer.StructureByteStride));
        memcpy(mMappedBuffer, bufferData, bufferSize); 
        break;
    case CPU_WRITE_GPU_READ_WRITE:
        Application::Assert(bufferSize <= (mUAVDesc.Buffer.NumElements * mUAVDesc.Buffer.StructureByteStride));

        {
            //copy data to the upload buffer
            void *mappedUpload = NULL;
            Direct3DUtils::ThrowIfHRESULTFailed(mUploadResource->Map(0, NULL, &mappedUpload));
            memcpy(mappedUpload, bufferData, bufferSize);
            mUploadResource->Unmap(0, NULL);
        }

        //then return that we need to schedule a GPU copy, but leave it up to the managing system to schedule when exactly
        needsGPUCopy = true;
        break;
    default:
        Application::Assert(false); //not implemented
        break;
    }

    return needsGPUCopy;
}

FilteredCubeMapRenderTexture::FilteredCubeMapRenderTexture(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32 numMipLevels, const DynamicArray<DescriptorHeapHandle> &uavHandles, DescriptorHeapHandle srvHandle)
    :GPUResource(resource, usageState)
{
    mSRVHandle = srvHandle;
    mNumMips = numMipLevels;
    mComputeFence = 0;

    for (uint32 i = 0; i < uavHandles.CurrentSize(); i++)
    {
        mUAVHandles.Add(uavHandles[i]);
    }
}

FilteredCubeMapRenderTexture::~FilteredCubeMapRenderTexture()
{

}

DescriptorHeapHandle FilteredCubeMapRenderTexture::GetUnorderedAccessViewHandle(uint32 mipIndex, uint32 arrayIndex /* = 0 */)
{
    return mUAVHandles[arrayIndex * mNumMips + mipIndex];
}

RayTraceBuffer::RayTraceBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, RayTraceBufferType bufferType)
    :GPUResource(resource, usageState)
{
    mGPUAddress = resource->GetGPUVirtualAddress();
    mBufferType = bufferType;
}

RayTraceBuffer::~RayTraceBuffer()
{

}

void RayTraceBuffer::MapInstanceDescData(const void *instanceDescData, uint32 numInstanceDescs)
{
    Application::Assert(mBufferType == RayTraceBufferType_Instancing);

    void *mappedData;
    mResource->Map(0, NULL, &mappedData);
    memcpy(mappedData, instanceDescData, numInstanceDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    mResource->Unmap(0, NULL);
}