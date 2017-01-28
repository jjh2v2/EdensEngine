#pragma once
#include "EdenEngineWindow.h"
#include "Core/CPU/GameTimer.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Render/Renderer/DeferredRenderer.h"
#include "Input/InputManager.h"
#include "Scene/SceneManager.h"

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
	SceneManager *mSceneManager;

	EdenEngineWindow *mEngineWindow;
	GameTimer *mGameTimer;
};