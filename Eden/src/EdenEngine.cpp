#include "EdenEngine.h"

EdenEngine::EdenEngine()
{
	mEngineWindow = new EdenEngineWindow(1920, 1080, false);
	mGameTimer = new GameTimer();
	testDXManager = new Direct3DManager();
	testDXManager->CreateWindowDependentResources(Vector2(1920, 1080), mEngineWindow->GetWindowHandle());
}

EdenEngine::~EdenEngine()
{
	delete mGameTimer;
	mGameTimer = NULL;

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

		shouldExit = mEngineWindow->ShouldQuit() || !Update(0.0167f) || !Render();
	}
}

void EdenEngine::OnScreenChanged()
{

}

bool EdenEngine::Update(float delta)
{
	return true;
}

bool EdenEngine::Render()
{
	return true;
}