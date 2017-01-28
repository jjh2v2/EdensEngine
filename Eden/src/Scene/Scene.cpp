#include "Scene/Scene.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Input/InputManager.h"

Scene::Scene(GraphicsManager *graphicsManager)
{
	mCameraManager = new CameraManager(graphicsManager->GetDirect3DManager()->GetScreenSize());
}

Scene::~Scene()
{
	delete mCameraManager;
}

void Scene::ApplyInput(InputManager *inputManager, float delta)
{
	Camera *mainCamera = mCameraManager->GetMainCamera();

	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_W))
	{
		mainCamera->MoveForward(delta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveForward(delta);
		}
	}
	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_S))
	{
		mainCamera->MoveBackward(delta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveBackward(delta);
		}
	}
	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_A))
	{
		mainCamera->MoveLeft(delta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveLeft(delta);
		}
	}
	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_D))
	{
		mainCamera->MoveRight(delta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveRight(delta);
		}
	}

	if (inputManager->IsRightMouseDown())
	{
		int32 mouseMoveX;
		int32 mouseMoveY;
		inputManager->GetMouseChange(mouseMoveX, mouseMoveY);
		mCameraManager->GetMainCamera()->MouseMove(mouseMoveX, mouseMoveY, delta);
	}
}

void Scene::Update(float delta)
{
	mCameraManager->Update(delta);
}