#pragma once

#include <d3d12.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include "Render/DirectX/D3D12Helper.h"
#include "Core/Platform/PlatformCore.h"
#include "Core/Containers/DynamicArray.h"
#include "Core/Vector/Vector2.h"
#include "Render/Display/DisplayOrientation.h"

class Direct3DManager
{
public:
	Direct3DManager();
	~Direct3DManager();

	void CreateWindowDependentResources(Vector2 screenSize, HWND windowHandle, bool vsync = false, bool fullScreen = false);
	void WaitForGPU();
	void Present();
	
	ID3D12Device*				GetDevice() { return mDevice; }
	IDXGISwapChain3*			GetSwapChain() { return mSwapChain; }
	ID3D12Resource*				GetBackBufferTarget() { return mBackBufferTargets[mCurrentBuffer]; }
	ID3D12CommandQueue*			GetCommandQueue() { return mCommandQueue; }
	ID3D12CommandAllocator*		GetCommandAllocator() { return mCommandAllocators[mCurrentBuffer]; }
	D3D12_VIEWPORT				GetScreenViewport() { return mScreenViewport; }

	void ThrowIfHRESULTFailed(HRESULT hr);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mCurrentBuffer, mRTVDescriptorSize);
	}

private:
	void InitializeDeviceResources();
	void ReleaseSwapChainDependentResources();
	void BuildSwapChainDependentResources();

	void MoveToNextFrame();

	DXGI_MODE_ROTATION ComputeDisplayRotation();

	uint32 mBufferCount;
	uint32 mCurrentBuffer;
	ID3D12Device *mDevice;
	IDXGIFactory4 *mDXGIFactory;
	IDXGISwapChain3	*mSwapChain;
	DynamicArray<ID3D12Resource*> mBackBufferTargets;
	ID3D12DescriptorHeap *mRTVHeap;
	uint32 mRTVDescriptorSize;
	ID3D12CommandQueue *mCommandQueue;
	DynamicArray<ID3D12CommandAllocator*> mCommandAllocators;
	D3D12_VIEWPORT	mScreenViewport;
	bool mDeviceRemoved;
	bool mUseVsync;
	bool mIsFullScreen;

	ID3D12Fence* mFence;
	DynamicArray<uint64> mFenceValues;
	HANDLE mFenceEvent;

	Vector2	mOutputSize;
	DisplayOrientation mNativeOrientation;
	DisplayOrientation mCurrentOrientation;
};