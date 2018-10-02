#pragma once
#include "Scene/Scene.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Input/InputManager.h"

class SceneManager
{
public:
	SceneManager(GraphicsManager *graphicsManager, InputManager *inputManager);
	~SceneManager();

	void OnScreenChanged(Vector2 screenSize);

	void Update(float delta);
    void UpdateInput(float delta);
	Scene *GetActiveScene() { return mActiveScene; }

private:
	GraphicsManager *mGraphicsManager;
	InputManager *mInputManager;

	DynamicArray<Scene*> mScenes;
	Scene *mActiveScene;
};