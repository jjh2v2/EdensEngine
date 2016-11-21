#include "Render/Buffer/GPUBuffer.h"

GPUBuffer::GPUBuffer()
{
}

GPUBuffer::~GPUBuffer()
{

}

void GPUBuffer::SetBufferInfo(size_t bufferSize, uint32 elementCount, uint32 elementSize, D3D12_RESOURCE_FLAGS flags)
{
	mBufferSize = bufferSize;
	mElementCount = elementCount;
	mElementSize = elementSize;
	mResourceFlags = flags;
}