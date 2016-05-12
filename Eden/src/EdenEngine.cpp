#include "EdenEngine.h"

EdenEngine::EdenEngine()
{
	mEngineWindow = new EdenEngineWindow(1920, 1080, false);
	mGameTimer = new GameTimer();

	mGraphicsManager = new GraphicsManager();
	mGraphicsManager->GetDirect3DManager()->CreateWindowDependentResources(Vector2(1920, 1080), mEngineWindow->GetEngineWindowHandle());

	mDeferredRenderer = new DeferredRenderer(mGraphicsManager);

	//TDA: change input manager when screen size changes, and get rid of the initialize function by moving the throw if failed to a common function
	mInputManager = new InputManager();
	mInputManager->Initialize(mEngineWindow->GetEngineModuleHandle(), mEngineWindow->GetEngineWindowHandle(), 1920, 1080);
}

EdenEngine::~EdenEngine()
{
	delete mInputManager;
	mInputManager = NULL;

	delete mGameTimer;
	mGameTimer = NULL;

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
		float frameTime = mGameTimer->GetTime();
		int FPS = mGameTimer->GetFPS();

		if (mEngineWindow->DidScreenChange())
		{
			OnScreenChanged();
		}

		//TDA: Use timer for update time
		shouldExit = mEngineWindow->ShouldQuit() || !Update(0.0167f) || !Render() || mInputManager->IsKeyboardKeyPressed(KeyboardKey_Escape);
	}
}

void EdenEngine::OnScreenChanged()
{
	//TDA: handle vsync and fullscreen changes
	mGraphicsManager->GetDirect3DManager()->CreateWindowDependentResources(mEngineWindow->GetWindowDimensions(), mEngineWindow->GetEngineWindowHandle());
}

bool EdenEngine::Update(float delta)
{
	mInputManager->Update();
	return true;
}

bool EdenEngine::Render()
{
	mDeferredRenderer->Render();
	mGraphicsManager->GetDirect3DManager()->Present();
	return true;
}