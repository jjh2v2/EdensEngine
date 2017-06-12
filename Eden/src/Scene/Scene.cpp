#include "Scene/Scene.h"
#include "Render/Graphics/GraphicsManager.h"
#include "Input/InputManager.h"

Scene::Scene(GraphicsManager *graphicsManager)
{
	mGraphicsManager = graphicsManager;
	mCameraManager = new CameraManager(mGraphicsManager->GetDirect3DManager()->GetScreenSize());
}

Scene::~Scene()
{
	delete mCameraManager;
}

void Scene::OnScreenChanged(Vector2 screenSize)
{
	mCameraManager->OnScreenChanged(screenSize);
}

void Scene::FreeSceneEntity(SceneEntity *entity)
{
	Material *material = entity->GetMaterial();
	mGraphicsManager->GetDirect3DManager()->GetContextManager()->FreeConstantBuffer(material->GetConstantBuffer());
	delete material;
	delete entity;
}

void Scene::ApplyInput(InputManager *inputManager, float delta)
{
	Camera *mainCamera = mCameraManager->GetMainCamera();
	const float cameraSpeedPerSecond = 20.0f;
	const float cameraDelta = cameraSpeedPerSecond * delta;

	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_W))
	{
		mainCamera->MoveForward(cameraDelta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveForward(cameraDelta);
		}
	}
	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_S))
	{
		mainCamera->MoveBackward(cameraDelta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveBackward(cameraDelta);
		}
	}
	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_A))
	{
		mainCamera->MoveLeft(cameraDelta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveLeft(cameraDelta);
		}
	}
	if (inputManager->IsKeyboardKeyPressed(KeyboardKey_D))
	{
		mainCamera->MoveRight(cameraDelta);
		if (inputManager->IsKeyboardKeyPressed(KeyboardKey_LeftShift))
		{
			mainCamera->MoveRight(cameraDelta);
		}
	}

	if (inputManager->IsRightMouseDown())
	{
		int32 mouseMoveX;
		int32 mouseMoveY;
		inputManager->GetMouseChange(mouseMoveX, mouseMoveY);
		mCameraManager->GetMainCamera()->MouseMove(mouseMoveX, mouseMoveY, cameraDelta);
	}
}

void Scene::Update(float delta)
{
	mCameraManager->Update(delta);
}