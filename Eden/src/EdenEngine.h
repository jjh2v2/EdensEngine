#pragma once
#include "EdenEngineWindow.h"
#include "Core/CPU/GameTimer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Render/Renderer/DeferredRenderer.h"
#include "Input/InputManager.h"

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

	GraphicsManager *mGraphicsManager;
	DeferredRenderer *mDeferredRenderer;
	InputManager *mInputManager;

	EdenEngineWindow *mEngineWindow;
	GameTimer *mGameTimer;
};