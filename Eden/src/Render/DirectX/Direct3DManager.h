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
#include "Render/DirectX/Context/Direct3DContextManager.h"

class Direct3DManager
{
public:
	Direct3DManager();
	~Direct3DManager();

	void CreateWindowDependentResources(Vector2 screenSize, HWND windowHandle, bool vsync = false, bool fullScreen = false);
	void WaitForGPU();
	void Present();
	
	ID3D12Device				*GetDevice() { return mDevice; }
	IDXGISwapChain3 			*GetSwapChain() { return mSwapChain; }
	RenderTarget  			    *GetBackBufferTarget() { return mBackBuffers[mCurrentBackBuffer]; }

	D3D12_VIEWPORT				GetScreenViewport() { return mScreenViewport; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView()
	{
		return mBackBufferHeapHandles[mCurrentBackBuffer].GetCPUHandle();
	}

	//D3D12_HEAP_PROPERTIES GetDefaultHeapProperties()  { return mDefaultHeapProperties; }
	//D3D12_HEAP_PROPERTIES GetUploadHeapProperties()   { return mUploadHeapProperties; }
	//D3D12_HEAP_PROPERTIES GetReadbackHeapProperties() { return mReadbackHeapProperties; }

	Direct3DContextManager *GetContextManager() { return mContextManager; }
	Direct3DHeapManager *GetHeapManager() { return mHeapManager; }

private:
	void InitializeDeviceResources();
	void ReleaseSwapChainDependentResources();
	void BuildSwapChainDependentResources();

	void MoveToNextFrame();

	DXGI_MODE_ROTATION ComputeDisplayRotation();
	
	ID3D12Device *mDevice;
	IDXGIFactory4 *mDXGIFactory;
	IDXGISwapChain3	*mSwapChain;
	DynamicArray<RenderTarget*> mBackBuffers;
	DynamicArray<DescriptorHeapHandle> mBackBufferHeapHandles;
	uint32 mCurrentBackBuffer;
	
	Direct3DContextManager *mContextManager;
	Direct3DHeapManager *mHeapManager;

	bool mDeviceRemoved;
	bool mUseVsync;
	bool mIsFullScreen;

	D3D12_VIEWPORT mScreenViewport;
	Vector2	mOutputSize;
	DisplayOrientation mNativeOrientation;
	DisplayOrientation mCurrentOrientation;

	//D3D12_HEAP_PROPERTIES mDefaultHeapProperties;
	//D3D12_HEAP_PROPERTIES mUploadHeapProperties;
	//D3D12_HEAP_PROPERTIES mReadbackHeapProperties;
};