#include "Render/DirectX/Direct3DManager.h"
#include <dxgidebug.h>

Direct3DManager::Direct3DManager()
{
	mDeviceRemoved = false;
	mNativeOrientation = DisplayOrientation_Landscape;
	mCurrentOrientation = DisplayOrientation_Landscape;
	mUseVsync = false;
	mContextManager = NULL;
	mCurrentBackBuffer = 0;

	InitializeDeviceResources();
}

Direct3DManager::~Direct3DManager()
{
	ReleaseSwapChainDependentResources();

	mSwapChain->Release();
	mSwapChain = NULL;

	delete mContextManager;
	mContextManager = NULL;

#ifdef _DEBUG
	IDXGIDebug1* pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif

#if defined(_DEBUG)
	mDebugController->Release();
	mDebugController = NULL;
#endif

	mDevice->Release();
	mDevice = NULL;

	mDXGIFactory->Release();
	mDXGIFactory = NULL;
}

void Direct3DManager::InitializeDeviceResources()
{
	
#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers.
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController))))
	{
		mDebugController->EnableDebugLayer();
	}
#endif

	Direct3DUtils::ThrowIfHRESULTFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mDXGIFactory)));

	// Create the Direct3D 12 API device object
	Direct3DUtils::ThrowIfHRESULTFailed(D3D12CreateDevice(
		nullptr,						// Specify nullptr to use the default adapter.
		D3D_FEATURE_LEVEL_11_0,			// Minimum feature level this app can support.
		IID_PPV_ARGS(&mDevice)		// Returns the Direct3D device created.
		));

	mContextManager = new Direct3DContextManager(mDevice);
}

void Direct3DManager::CreateWindowDependentResources(Vector2 screenSize, HWND windowHandle, bool vsync /*= false*/, bool fullScreen /*= false*/)
{
	// Wait until all previous GPU work is complete.
	mContextManager->GetGraphicsContext()->Flush(mContextManager->GetQueueManager(), true);

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
		HRESULT hr = mSwapChain->ResizeBuffers(BACK_BUFFER_COUNT, lround(mOutputSize.X), lround(mOutputSize.Y), DXGI_FORMAT_R8G8B8A8_UNORM, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			mDeviceRemoved = true;
			return;
		}
		else
		{
			Direct3DUtils::ThrowIfHRESULTFailed(hr);
		}
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		IDXGIAdapter* adapter = NULL;
		IDXGIOutput* adapterOutput = NULL;
		uint32 numDisplayModes = 0;

		Direct3DUtils::ThrowIfHRESULTFailed(mDXGIFactory->EnumAdapters(0, &adapter));
		Direct3DUtils::ThrowIfHRESULTFailed(adapter->EnumOutputs(0, &adapterOutput));
		Direct3DUtils::ThrowIfHRESULTFailed(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, NULL));
		DXGI_MODE_DESC *displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		Direct3DUtils::ThrowIfHRESULTFailed(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList));
		
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
		swapChainDesc.Width = lround(mOutputSize.X);
		swapChainDesc.Height = lround(mOutputSize.Y);
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
		Direct3DUtils::ThrowIfHRESULTFailed(
			mDXGIFactory->CreateSwapChainForHwnd(
				mContextManager->GetQueueManager()->GetGraphicsQueue()->GetCommandQueue(),
				windowHandle,
				&swapChainDesc,
				&swapChainFullScreenDesc,
				NULL,
				&swapChain
				)
			);
		
		Direct3DUtils::ThrowIfHRESULTFailed(swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&mSwapChain));
	}

	Direct3DUtils::ThrowIfHRESULTFailed(mSwapChain->SetRotation(displayRotation));

	BuildSwapChainDependentResources();

	// Set the 3D rendering viewport to target the entire window.
	mScreenViewport = { 0.0f, 0.0f, mOutputSize.X, mOutputSize.Y, 0.0f, 1.0f };

	mContextManager->GetGraphicsContext()->Flush(mContextManager->GetQueueManager(), true);
}

void Direct3DManager::ReleaseSwapChainDependentResources()
{
	for (uint32 bufferIndex = 0; bufferIndex < BACK_BUFFER_COUNT; bufferIndex++)
	{
		mContextManager->GetHeapManager()->FreeRTVDescriptorHeapHandle(mBackBuffers[bufferIndex]->GetRenderTargetViewHandle());
		delete mBackBuffers[bufferIndex];
		mBackBuffers[bufferIndex] = NULL;
	}

	mBackBuffers.Clear();
}

void Direct3DManager::BuildSwapChainDependentResources()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = BACK_BUFFER_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	
	for (uint32 bufferIndex = 0; bufferIndex < BACK_BUFFER_COUNT; bufferIndex++)
	{
		ID3D12Resource *backBufferResource = NULL;
		DescriptorHeapHandle backBufferHeapHandle = mContextManager->GetHeapManager()->GetNewRTVDescriptorHeapHandle();
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		Direct3DUtils::ThrowIfHRESULTFailed(mSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&backBufferResource)));
		mDevice->CreateRenderTargetView(backBufferResource, &rtvDesc, backBufferHeapHandle.GetCPUHandle());

		mBackBuffers.Add(new BackBufferTarget(backBufferResource, D3D12_RESOURCE_STATE_PRESENT, backBufferHeapHandle));

		WCHAR name[25];
		swprintf_s(name, L"Backbuffer Target %d", bufferIndex);
		mBackBuffers[bufferIndex]->GetResource()->SetName(name);
	}

	mCurrentBackBuffer = 0;
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
		Direct3DUtils::ThrowIfHRESULTFailed(hr);
		MoveToNextFrame();
	}
}

void Direct3DManager::MoveToNextFrame()
{
	// Advance the frame index.
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % BACK_BUFFER_COUNT;
	uint64 fenceValue = mContextManager->GetQueueManager()->GetGraphicsQueue()->IncrementFence();
	mContextManager->GetQueueManager()->GetGraphicsQueue()->WaitForFence(fenceValue);
}

DXGI_MODE_ROTATION Direct3DManager::ComputeDisplayRotation()
{
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