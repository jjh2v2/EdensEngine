#pragma once

#include "Core/Platform/PlatformCore.h"
#include "Render/DirectX/D3D12Helper.h"
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
	void Present();
	
	ID3D12Device	 *GetDevice() { return mDevice; }
    ID3D12Device5    *GetRayTraceDevice() { return mDXRDevice; }
	IDXGISwapChain3  *GetSwapChain() { return mSwapChain; }
	BackBufferTarget *GetBackBufferTarget() { return mBackBuffers[mCurrentBackBuffer]; }

	D3D12_VIEWPORT	GetScreenViewport() { return mScreenViewport; }
	Vector2			GetScreenSize() { return mOutputSize; }
    uint32          GetFrameIndex() { return mCurrentBackBuffer; }

	Direct3DContextManager *GetContextManager() { return mContextManager; }

    bool IsDXRSupported() { return mSupportsDXR; }

private:
	void InitializeDeviceResources();
	void ReleaseSwapChainDependentResources();
	void BuildSwapChainDependentResources();

	void MoveToNextFrame();

	DXGI_MODE_ROTATION ComputeDisplayRotation();
	
	ID3D12Device  *mDevice;
    ID3D12Device5 *mDXRDevice;
	IDXGIFactory4 *mDXGIFactory;
	IDXGISwapChain3	*mSwapChain;
	DynamicArray<BackBufferTarget*> mBackBuffers;
	uint32 mCurrentBackBuffer;
	
	Direct3DContextManager *mContextManager;

	bool mDeviceRemoved;
	bool mUseVsync;
	bool mIsFullScreen;
    bool mSupportsDXR;

	D3D12_VIEWPORT mScreenViewport;
	Vector2	mOutputSize;
	DisplayOrientation mNativeOrientation;
	DisplayOrientation mCurrentOrientation;
};