#include "EdenEngine.h"

EdenEngine::EdenEngine()
{
	mEngineWindow = new EdenEngineWindow(1920, 1080, false);
}

EdenEngine::~EdenEngine()
{
	delete mEngineWindow;
	mEngineWindow = NULL;
}

void EdenEngine::Run()
{
	bool shouldExit = false;
	while (!shouldExit)
	{
		//mGameTimer->Frame();

		//float frameTime = mGameTimer->GetTime();
		//int FPS = mGameTimer->GetFPS();

		shouldExit = mEngineWindow->ShouldQuit() || !Update(0.0167f) || !Render();
	}
}

bool EdenEngine::Update(float delta)
{
	return true;
}

bool EdenEngine::Render()
{
	return true;
}