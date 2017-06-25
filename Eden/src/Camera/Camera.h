#pragma once
#include "Core/Vector/Vector3.h"
#include "Core/Misc/Box.h"

class Camera
{
public:
	struct CameraScreenSettings
	{
		CameraScreenSettings()
		{
			Width = 0;
			Height = 0;
			Near = 0;
			Far = 0;
			AspectRatio = 0;
			FieldOfView = 0;
		}

		float Width;
		float Height;
		float Near;
		float Far;
		float AspectRatio;
		float FieldOfView;
	};

	Camera(CameraScreenSettings screenSettings, float moveSpeed, float moveSensitivity);
	~Camera();

	void OnScreenChanged(CameraScreenSettings screenSettings);
    CameraScreenSettings GetScreenSettings() { return mScreenSettings; }

	Vector3 GetPosition() { return mPosition; }
	Vector3 GetRotation() { return mRotation; }
	Vector3 GetDirection();
	void SetPosition(Vector3 position);
	void SetRotation(Vector3 rotation);

	void RebuildViewMatrix();

	void MoveForward(float speedMult);
	void MoveBackward(float speedMult);
	void MoveLeft(float speedMult);
	void MoveRight(float speedMult);
	void MouseMove(int32 mouseX, int32 mouseY, float speedMult);

	const D3DXMATRIX& GetViewMatrix() { return mViewMatrix; }
	const D3DXMATRIX& GetProjectionMatrix() { return mProjectionMatrix; }
	const D3DXMATRIX& GetReverseProjectionMatrix() { return mReverseProjectionMatrix; }

	bool IsBoxInView(Box box);
	bool IsSphereInView(float xCenter, float yCenter, float zCenter, float radius);

private:
	void RebuildFrustum();

	CameraScreenSettings mScreenSettings;

	Vector3 mPosition;
	Vector3 mRotation;
	D3DXMATRIX mViewMatrix;
	D3DXMATRIX mProjectionMatrix;
	D3DXMATRIX mReverseProjectionMatrix;
	D3DXPLANE mFrustumPlanes[6];

	float mMoveSpeed;
	float mMoveSensitivity;
};