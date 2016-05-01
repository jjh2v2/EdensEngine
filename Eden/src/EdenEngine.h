#pragma once
#include "EdenEngineWindow.h"
#include "Core/CPU/GameTimer.h"

class EdenEngine
{
public:
	EdenEngine();
	~EdenEngine();
	void Run();

private:
	bool Update(float delta);
	bool Render();

	EdenEngineWindow *mEngineWindow;
	GameTimer *mGameTimer;
};