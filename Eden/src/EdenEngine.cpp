#include "EdenEngine.h"

EdenEngine::EdenEngine()
{
	mEngineWindow = new EdenEngineWindow(1920, 1080, false);
	mGameTimer = new GameTimer();
	mGraphicsManager = new GraphicsManager();
	mGraphicsManager->GetDirect3DManager()->CreateWindowDependentResources(Vector2(1920, 1080), mEngineWindow->GetWindowHandle());

	mDeferredRenderer = new DeferredRenderer(mGraphicsManager);
}

EdenEngine::~EdenEngine()
{
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
		shouldExit = mEngineWindow->ShouldQuit() || !Update(0.0167f) || !Render();
	}
}

void EdenEngine::OnScreenChanged()
{
	//TDA: handle vsync and fullscreen changes
	mGraphicsManager->GetDirect3DManager()->CreateWindowDependentResources(mEngineWindow->GetWindowDimensions(), mEngineWindow->GetWindowHandle());
}

bool EdenEngine::Update(float delta)
{
	return true;
}

bool EdenEngine::Render()
{
	mGraphicsManager->GetDirect3DManager()->Present();
	return false;
}