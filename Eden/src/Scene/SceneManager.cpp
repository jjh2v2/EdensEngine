#include "Scene/SceneManager.h"

SceneManager::SceneManager(GraphicsManager *graphicsManager, InputManager *inputManager)
{
	mGraphicsManager = graphicsManager;
	mInputManager = inputManager;

	mScenes.Add(new Scene(mGraphicsManager));
	mActiveScene = mScenes[0];
}

SceneManager::~SceneManager()
{
	for (uint32 i = 0; i < mScenes.CurrentSize(); i++)
	{
		delete mScenes[i];
	}
}

void SceneManager::OnScreenChanged(Vector2 screenSize)
{
	mActiveScene->OnScreenChanged(screenSize);
}

void SceneManager::Update(float delta)
{
    mActiveScene->Update(delta);
}

void SceneManager::UpdateInput(float delta)
{
    mActiveScene->ApplyInput(mInputManager, delta);
    mActiveScene->UpdateCamera(delta);
}