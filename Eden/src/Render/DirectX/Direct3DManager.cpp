#include "Render/DirectX/Direct3DManager.h"

Direct3DManager::Direct3DManager()
{
	mBufferCount = DEFAULT_BUFFERING_COUNT;
	mDevice = NULL;
	mDXGIFactory = NULL;
	mSwapChain = NULL;
	mRTVHeap = NULL;
	mRTVDescriptorSize = 0;
	mCommandQueue = NULL;
	mDeviceRemoved = false;
    mFence = NULL;
	mFenceEvent = NULL;
	mNativeOrientation = DisplayOrientation_Landscape;
	mCurrentOrientation = DisplayOrientation_Landscape;
	mUseVsync = false;
	mCommandAllocator = NULL;

	for (uint32 bufferIndex = 0; bufferIndex < mBufferCount; bufferIndex++)
	{
		mFenceValues.Add(0);
		mBackBufferTargets.Add(NULL);
	}

	InitializeDeviceResources();
}

Direct3DManager::~Direct3DManager()
{
	ReleaseSwapChainDependentResources();

	mSwapChain->Release();
	mSwapChain = NULL;

	mFence->Release();
	mFence = NULL;
	
	mCommandAllocator->Release();
	mCommandAllocator = NULL;

	mCommandQueue->Release();
	mCommandQueue = NULL;

	mDevice->Release();
	mDevice = NULL;

	mDXGIFactory->Release();
	mDXGIFactory = NULL;
}

void Direct3DManager::InitializeDeviceResources()
{
	
#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers.
{
	ID3D12Debug *debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	debugController->Release();
	debugController = NULL;
}
#endif

	ThrowIfHRESULTFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mDXGIFactory)));

	// Create the Direct3D 12 API device object
	ThrowIfHRESULTFailed(D3D12CreateDevice(
		nullptr,						// Specify nullptr to use the default adapter.
		D3D_FEATURE_LEVEL_11_0,			// Minimum feature level this app can support.
		IID_PPV_ARGS(&mDevice)		// Returns the Direct3D device created.
		));

	// Create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.NodeMask = 0;

	ThrowIfHRESULTFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfHRESULTFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));

	// Create synchronization objects.
	ThrowIfHRESULTFailed(mDevice->CreateFence(mFenceValues[mCurrentBuffer], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	mFenceValues[mCurrentBuffer]++;

	mFenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
}

void Direct3DManager::CreateWindowDependentResources(Vector2 screenSize, HWND windowHandle, bool vsync /*= false*/, bool fullScreen /*= false*/)
{
	// Wait until all previous GPU work is complete.
	WaitForGPU();

	mOutputSize = screenSize;
	mUseVsync = vsync;				//need to handle if vsync or fullscreen changes
	mIsFullScreen = fullScreen;

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	mOutputSize.X = swapDimensions ? screenSize.Y : screenSize.X;
	mOutputSize.Y = swapDimensions ? screenSize.X : screenSize.Y;

	if (mSwapChain != NULL)
	{
		ReleaseSwapChainDependentResources();
		// If the swap chain already exists, resize it.
		HRESULT hr = mSwapChain->ResizeBuffers(mBufferCount, lround(mOutputSize.X), lround(mOutputSize.Y), DXGI_FORMAT_R8G8B8A8_UNORM, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			mDeviceRemoved = true;

			// Do not continue execution of this method. DeviceResources will be destroyed and re-created.
			return;
		}
		else
		{
			ThrowIfHRESULTFailed(hr);
		}
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		IDXGIAdapter* adapter = NULL;
		IDXGIOutput* adapterOutput = NULL;
		uint32 numDisplayModes = 0;

		ThrowIfHRESULTFailed(mDXGIFactory->EnumAdapters(0, &adapter));
		ThrowIfHRESULTFailed(adapter->EnumOutputs(0, &adapterOutput));
		ThrowIfHRESULTFailed(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, NULL));
		DXGI_MODE_DESC *displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		ThrowIfHRESULTFailed(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList));
		
		uint32 numerator = 0;
		uint32 denominator = 0;

		for (uint32 i = 0; i < numDisplayModes; i++)
		{
			if (displayModeList[i].Height == (uint32)mOutputSize.X)
			{
				if (displayModeList[i].Width == (uint32)mOutputSize.Y)
				{
					numerator = displayModeList[i].RefreshRate.Numerator;
					denominator = displayModeList[i].RefreshRate.Denominator;
				}
			}
		}

		/*
		DXGI_ADAPTER_DESC adapterDesc;
		// Get the adapter (video card) description.
		result = adapter->GetDesc(&adapterDesc);
		if(FAILED(result))
		{
			return false;
		}

		// Store the dedicated video card memory in megabytes.
		m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

		// Convert the name of the video card to a character array and store it.
		error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
		if(error != 0)
		{
			return false;
		}
		*/
		delete[] displayModeList;
		displayModeList = NULL;
		adapterOutput->Release();
		adapterOutput = NULL;
		adapter->Release();
		adapter = NULL;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.Width = lround(mOutputSize.X);	// Match the size of the window.
		swapChainDesc.Height = lround(mOutputSize.Y);
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// This is the most common swap chain format.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;							// Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = mBufferCount;					// Use triple-buffering to minimize latency.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// All Windows Universal apps must use _FLIP_ SwapEffects
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc = {};
		ZeroMemory(&swapChainFullScreenDesc, sizeof(swapChainFullScreenDesc));
		swapChainFullScreenDesc.Windowed = !mIsFullScreen;
		swapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		
		if (mUseVsync)
		{
			swapChainFullScreenDesc.RefreshRate.Numerator = numerator;
			swapChainFullScreenDesc.RefreshRate.Denominator = denominator;
		}
		else
		{
			swapChainFullScreenDesc.RefreshRate.Numerator = 0;
			swapChainFullScreenDesc.RefreshRate.Denominator = 1;
		}


		IDXGISwapChain1 *swapChain = NULL;
		ThrowIfHRESULTFailed(
			mDXGIFactory->CreateSwapChainForHwnd(
				mCommandQueue,
				windowHandle,
				&swapChainDesc,
				&swapChainFullScreenDesc,
				nullptr,
				&swapChain
				)
			);
		
		ThrowIfHRESULTFailed(swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&mSwapChain));
	}

	// Set the proper orientation for the swap chain, and generate
	// 3D matrix transformations for rendering to the rotated swap chain.
	// The 3D matrix is specified explicitly to avoid rounding errors.

	/*switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		throw ref new FailureException();
	}*/

	ThrowIfHRESULTFailed(mSwapChain->SetRotation(displayRotation));

	BuildSwapChainDependentResources();

	// Set the 3D rendering viewport to target the entire window.
	mScreenViewport = { 0.0f, 0.0f, mOutputSize.X, mOutputSize.Y, 0.0f, 1.0f };
}

void Direct3DManager::ReleaseSwapChainDependentResources()
{
	for (uint32 bufferIndex = 0; bufferIndex < mBufferCount; bufferIndex++)
	{
		mBackBufferTargets[bufferIndex]->Release();
		mBackBufferTargets[bufferIndex] = NULL;
	}

	mRTVHeap->Release();
	mRTVHeap = NULL;

	// All pending GPU work was already finished. Update the tracked fence values
	// to the last value signaled.
	//for (uint32 bufferIndex = 0; bufferIndex < mBufferCount; bufferIndex++)
	//{
	//	mFenceValues[bufferIndex] = mFenceValues[mCurrentBuffer];
	//}
}

void Direct3DManager::BuildSwapChainDependentResources()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = mBufferCount;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfHRESULTFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mRTVHeap)));
	mRTVHeap->SetName(L"Backbuffer Descriptor Heap");

	// All pending GPU work was already finished. Update the tracked fence values
	// to the last value signaled.
	for (uint32 bufferIndex = 0; bufferIndex < mBufferCount; bufferIndex++)
	{
		mFenceValues[bufferIndex] = mFenceValues[mCurrentBuffer];
	}

	mCurrentBuffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
	mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (uint32 bufferIndex = 0; bufferIndex < mBufferCount; bufferIndex++)
	{
		//TDA: we need a descriptor here, especially to use an SRGB target
		ThrowIfHRESULTFailed(mSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&mBackBufferTargets[bufferIndex])));
		mDevice->CreateRenderTargetView(mBackBufferTargets[bufferIndex], nullptr, rtvDescriptor);
		rtvDescriptor.Offset(mRTVDescriptorSize);

		WCHAR name[25];
		swprintf_s(name, L"Backbuffer Target %d", bufferIndex);
		mBackBufferTargets[bufferIndex]->SetName(name);
	}
}

void Direct3DManager::WaitForGPU()
{
	// Schedule a Signal command in the queue.
	ThrowIfHRESULTFailed(mCommandQueue->Signal(mFence, mFenceValues[mCurrentBuffer]));

	// Wait until the fence has been crossed.
	ThrowIfHRESULTFailed(mFence->SetEventOnCompletion(mFenceValues[mCurrentBuffer], mFenceEvent));
	WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	mFenceValues[mCurrentBuffer]++;
}

// Present the contents of the swap chain to the screen.
void Direct3DManager::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = mSwapChain->Present(mUseVsync ? 1 : 0, 0);

	// If the device was removed either by a disconnection or a driver upgrade, we 
	// must recreate all device resources.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		mDeviceRemoved = true;
	}
	else
	{
		ThrowIfHRESULTFailed(hr);
		MoveToNextFrame();
	}
}

void Direct3DManager::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = mFenceValues[mCurrentBuffer];
	ThrowIfHRESULTFailed(mCommandQueue->Signal(mFence, currentFenceValue));

	// Advance the frame index.
	mCurrentBuffer = (mCurrentBuffer + 1) % mBufferCount;

	// Check to see if the next frame is ready to start.
	if (mFence->GetCompletedValue() < mFenceValues[mCurrentBuffer])
	{
		ThrowIfHRESULTFailed(mFence->SetEventOnCompletion(mFenceValues[mCurrentBuffer], mFenceEvent));
		WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	mFenceValues[mCurrentBuffer] = currentFenceValue + 1;
}

DXGI_MODE_ROTATION Direct3DManager::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// Note: NativeOrientation can only be Landscape or Portrait even though
	// the DisplayOrientations enum has other values.
	switch (mNativeOrientation)
	{
	case DisplayOrientation_Landscape:
		switch (mCurrentOrientation)
		{
		case DisplayOrientation_Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientation_Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientation_Landscape_Flipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DisplayOrientation_Portrait_Flipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DisplayOrientation_Portrait:
		switch (mCurrentOrientation)
		{
		case DisplayOrientation_Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DisplayOrientation_Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientation_Landscape_Flipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientation_Portrait_Flipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}

	return rotation;
}

void Direct3DManager::ThrowIfHRESULTFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		// Set a breakpoint on this line to catch Win32 API errors.
		throw std::runtime_error("Device operation failed.");
	}
}