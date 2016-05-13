#pragma once
#include "Core/Platform/PlatformCore.h"
#include "Asset/DDS/DDSInfo.h"

class DDSLoader
{
public:
	DDSLoader();
	~DDSLoader();

private:
	bool IsBitMask(DirectX::DDS_PIXELFORMAT pixelFormat, uint32 r, uint32 g, uint32 b, uint32 a);
	DXGI_FORMAT GetDXGIFormat(const DirectX::DDS_PIXELFORMAT pixelFormat);
	DXGI_FORMAT MakeSRGB(DXGI_FORMAT format);
	size_t GetBitsPerPixel(DXGI_FORMAT format);
	void GetSurfaceInfo(size_t width, size_t height, DXGI_FORMAT format, size_t* outNumBytes, size_t* outRowBytes, size_t* outNumRows);
	HRESULT FillInitData(size_t width, size_t height, size_t depth, size_t mipCount, size_t arraySize,
		DXGI_FORMAT format, size_t maxsize, size_t bitSize, _In_reads_bytes_(bitSize) const uint8_t* bitData,
		size_t& twidth, size_t& theight, size_t& tdepth, size_t& skipMip, _Out_writes_(mipCount*arraySize) D3D12_SUBRESOURCE_DATA* initData);
	HRESULT CreateD3DResources(_In_ ID3D12Device* d3dDevice, _In_ uint32_t resDim, _In_ size_t width, _In_ size_t height,
		_In_ size_t depth, _In_ size_t mipCount, _In_ size_t arraySize, _In_ DXGI_FORMAT format, _In_ bool forceSRGB, _In_ bool isCubeMap,
		_In_reads_opt_(mipCount*arraySize) D3D12_SUBRESOURCE_DATA* initData, _Outptr_opt_ ID3D12Resource** texture, _In_ D3D12_CPU_DESCRIPTOR_HANDLE textureView);
	HRESULT CreateTextureFromDDS(_In_ ID3D12Device* d3dDevice, _In_ const DirectX::DDS_HEADER* header, _In_reads_bytes_(bitSize) const uint8_t* bitData,
		_In_ size_t bitSize, _In_ size_t maxsize, _In_ bool forceSRGB, _Outptr_opt_ ID3D12Resource** texture, _In_ D3D12_CPU_DESCRIPTOR_HANDLE textureView)
};