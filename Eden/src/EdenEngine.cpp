#include "EdenEngine.h"
#include "Core/Threading/ThreadPoolManager.h"

EdenEngine::EdenEngine()
{
    uint32 numGeneralWorkers = CPUDeviceInfo::GetNumberOfLogicalCores() - 1; //# of cores minus 1 (the main thread)
    uint32 numBackgroundWorkers = 1;  //just one for now, for loading and other work. Will probably add another one just for IO eventually.

    ThreadPoolManager::GetSingleton()->Initialize(numGeneralWorkers, numBackgroundWorkers);

	mEngineWindow = new EdenEngineWindow(1920, 1080, false);
	mGameTimer = new GameTimer();

	mGraphicsManager = new GraphicsManager();
	mGraphicsManager->GetDirect3DManager()->CreateWindowDependentResources(Vector2(1920, 1080), mEngineWindow->GetEngineWindowHandle());
	mGraphicsManager->InitializeGraphicsResources();
	mDeferredRenderer = new DeferredRenderer(mGraphicsManager);

	mInputManager = new InputManager(mEngineWindow->GetEngineModuleHandle(), mEngineWindow->GetEngineWindowHandle(), 1920, 1080);

	mSceneManager = new SceneManager(mGraphicsManager, mInputManager);
	mDeferredRenderer->SetActiveScene(mSceneManager->GetActiveScene());
}

EdenEngine::~EdenEngine()
{
    ThreadPoolManager::DestroySingleton();

	delete mSceneManager;
	mSceneManager = NULL;

	delete mInputManager;
	mInputManager = NULL;

	delete mGameTimer;
	mGameTimer = NULL;

    mGraphicsManager->FlushAllAndWaitForIdle();

	delete mDeferredRenderer;
	mDeferredRenderer = NULL;

	delete mGraphicsManager;
	mGraphicsManager = NULL;

	delete mEngineWindow;
	mEngineWindow = NULL;
}

void EdenEngine::Run()
{
	bool shouldExit = false;
	while (!shouldExit)
	{
		mGameTimer->Frame();
		int32 FPS = mGameTimer->GetFPS();
		
		if (mEngineWindow->DidScreenChange())
		{
			OnScreenChanged();
		}

		shouldExit = mEngineWindow->ShouldQuit() || !Update(mGameTimer->GetTimeSeconds()) || !Render() || mInputManager->IsKeyboardKeyPressed(KeyboardKey_Escape);
	}
}

void EdenEngine::OnScreenChanged()
{
	//TDA: handle vsync and fullscreen changes
	Vector2 screenDimensions = mEngineWindow->GetWindowDimensions();

	mGraphicsManager->GetDirect3DManager()->CreateWindowDependentResources(screenDimensions, mEngineWindow->GetEngineWindowHandle());
	mDeferredRenderer->OnScreenChanged(screenDimensions);
	mInputManager->OnScreenChanged((int32)screenDimensions.X, (int32)screenDimensions.Y);
	mSceneManager->OnScreenChanged(screenDimensions);
}

bool EdenEngine::Update(float delta)
{
    mGraphicsManager->Update(delta);
	mInputManager->Update();

	if (mInputManager->GetRightClickDown())
	{
		mEngineWindow->ShowWindowCursor(false);
	}
	else if (mInputManager->GetRightClickUp())
	{
		mEngineWindow->ShowWindowCursor(true);
	}

	mSceneManager->Update(delta);
	return true;
}

bool EdenEngine::Render()
{
	mDeferredRenderer->Render();
	mGraphicsManager->GetDirect3DManager()->Present();
	return true;
}