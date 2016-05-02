#pragma once
#include "EdenEngineWindow.h"
#include "Core/CPU/GameTimer.h"
#include "Render/DirectX/Direct3DManager.h"

class EdenEngine
{
public:
	EdenEngine();
	~EdenEngine();
	void Run();

private:
	bool Update(float delta);
	bool Render();
	void OnScreenChanged();

	Direct3DManager *testDXManager;

	EdenEngineWindow *mEngineWindow;
	GameTimer *mGameTimer;
};