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
	BackBufferTarget  			*GetBackBufferTarget() { return mBackBuffers[mCurrentBackBuffer]; }

	D3D12_VIEWPORT				GetScreenViewport() { return mScreenViewport; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView()
	{
		return mBackBuffers[mCurrentBackBuffer]->GetRenderTargetViewHandle().GetCPUHandle();
	}

	Direct3DContextManager *GetContextManager() { return mContextManager; }

private:
	void InitializeDeviceResources();
	void ReleaseSwapChainDependentResources();
	void BuildSwapChainDependentResources();

	void MoveToNextFrame();

	DXGI_MODE_ROTATION ComputeDisplayRotation();
	
	ID3D12Device *mDevice;
	IDXGIFactory4 *mDXGIFactory;
	IDXGISwapChain3	*mSwapChain;
	DynamicArray<BackBufferTarget*> mBackBuffers;
	uint32 mCurrentBackBuffer;
	
	Direct3DContextManager *mContextManager;

	bool mDeviceRemoved;
	bool mUseVsync;
	bool mIsFullScreen;

	D3D12_VIEWPORT mScreenViewport;
	Vector2	mOutputSize;
	DisplayOrientation mNativeOrientation;
	DisplayOrientation mCurrentOrientation;
};