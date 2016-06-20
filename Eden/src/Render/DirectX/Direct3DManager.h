#pragma once

#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include "Render/DirectX/D3D12Helper.h"
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Core/Vector/Vector2.h"
#include "Render/Display/DisplayOrientation.h"
#include "Render/DirectX/Resources/Direct3DResources.h"
#include "Render/DirectX/Heap/Direct3DHeapManager.h"
#include "Render/DirectX/Upload/Direct3DUploadManager.h"

class Direct3DManager
{
public:
	Direct3DManager();
	~Direct3DManager();

	void CreateWindowDependentResources(Vector2 screenSize, HWND windowHandle, bool vsync = false, bool fullScreen = false);
	void WaitForGPU();
	void Present();
	
	ID3D12Device				*GetDevice() { return mDirect3DResources.Device; }
	IDXGISwapChain3 			*GetSwapChain() { return mDirect3DResources.SwapChain; }
	ID3D12Resource  			*GetBackBufferTarget() { return mDirect3DResources.BackBufferTargets[mDirect3DResources.CurrentBuffer]; }
	ID3D12CommandQueue			*GetCommandQueue() { return mDirect3DResources.CommandQueue; }
	ID3D12CommandAllocator		*GetCommandAllocator() { return mDirect3DResources.CommandAllocators[mDirect3DResources.CurrentBuffer]; }
	ID3D12GraphicsCommandList   *GetCommandList() { return mDirect3DResources.CommandList; }

	D3D12_VIEWPORT				GetScreenViewport() { return mDirect3DResources.ScreenViewport; }

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDirect3DResources.RTVHeap->GetCPUDescriptorHandleForHeapStart(), mDirect3DResources.CurrentBuffer, mDirect3DResources.RTVDescriptorSize);
	}

	D3D12_HEAP_PROPERTIES GetDefaultHeapProperties()  { return mDefaultHeapProperties; }
	D3D12_HEAP_PROPERTIES GetUploadHeapProperties()   { return mUploadHeapProperties; }
	D3D12_HEAP_PROPERTIES GetReadbackHeapProperties() { return mReadbackHeapProperties; }

	Direct3DHeapManager *GetHeapManager() { return mHeapManager; }
	Direct3DUploadManager *GetUploadManager() { return mUploadManager; }

private:
	void InitializeDeviceResources();
	void ReleaseSwapChainDependentResources();
	void BuildSwapChainDependentResources();

	void MoveToNextFrame();

	DXGI_MODE_ROTATION ComputeDisplayRotation();

	Direct3DResources mDirect3DResources;
	Direct3DHeapManager *mHeapManager;
	Direct3DUploadManager *mUploadManager;

	bool mDeviceRemoved;
	bool mUseVsync;
	bool mIsFullScreen;

	Vector2	mOutputSize;
	DisplayOrientation mNativeOrientation;
	DisplayOrientation mCurrentOrientation;

	D3D12_HEAP_PROPERTIES mDefaultHeapProperties;
	D3D12_HEAP_PROPERTIES mUploadHeapProperties;
	D3D12_HEAP_PROPERTIES mReadbackHeapProperties;
};