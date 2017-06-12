#include "Camera/Camera.h"

Camera::Camera(CameraScreenSettings screenSettings, float moveSpeed, float moveSensitivity)
{
	mPosition = Vector3(0, 0, 0);
	mRotation = Vector3(0, 0, 0);
	mMoveSpeed = moveSpeed;
	mMoveSensitivity = moveSensitivity;

	OnScreenChanged(screenSettings);
}

Camera::~Camera()
{

}

void Camera::OnScreenChanged(CameraScreenSettings screenSettings)
{
	mScreenSettings = screenSettings;

	D3DXMatrixPerspectiveFovLH(&mProjectionMatrix, mScreenSettings.FieldOfView, mScreenSettings.AspectRatio, mScreenSettings.Near, mScreenSettings.Far);
	D3DXMatrixPerspectiveFovLH(&mReverseProjectionMatrix, mScreenSettings.FieldOfView, mScreenSettings.AspectRatio, mScreenSettings.Far, mScreenSettings.Near);
	RebuildViewMatrix();
}

void Camera::MoveForward(float speedMult)
{
	mPosition.X += sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
	mPosition.Y -= sinf(mRotation.X * MathHelper::Radian()) * mMoveSpeed * speedMult;
	mPosition.Z += cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
}

void Camera::MoveBackward(float speedMult)
{
	mPosition.X -= sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
	mPosition.Y += sinf(mRotation.X * MathHelper::Radian()) * mMoveSpeed * speedMult;
	mPosition.Z -= cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
}

void Camera::MoveLeft(float speedMult)
{
	mPosition.Z += sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
	mPosition.X -= cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
}

void Camera::MoveRight(float speedMult)
{
	mPosition.Z -= sinf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
	mPosition.X += cosf(mRotation.Y * MathHelper::Radian()) * mMoveSpeed * speedMult;
}

void Camera::MouseMove(int32 mouseX, int32 mouseY, float speedMult)
{
	mRotation.Y += mouseX * MathHelper::Radian() * mMoveSensitivity * speedMult;
	mRotation.X += mouseY * MathHelper::Radian() * mMoveSensitivity * speedMult;
}

Vector3 Camera::GetDirection()
{
	return Vector3(
		sinf(mRotation.Y * MathHelper::Radian()),
	   -sinf(mRotation.X * MathHelper::Radian()),
		cosf(mRotation.Y * MathHelper::Radian()));
}

void Camera::SetPosition(Vector3 position)
{
	mPosition = position;
}

void Camera::SetRotation(Vector3 rotation)
{
	mRotation = rotation;
}

void Camera::RebuildViewMatrix()
{
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXVECTOR3 position = mPosition.AsD3DVector3();
	D3DXVECTOR3 lookAt(0.0f, 0.0f, 1.0f);
	float yaw = mRotation.Y * MathHelper::Radian();
	float pitch = mRotation.X * MathHelper::Radian();
	float roll = mRotation.Z * MathHelper::Radian();
	D3DXMATRIX rotationMatrix;

	D3DXMatrixRotationYawPitchRoll(&rotationMatrix, yaw, pitch, roll);
	D3DXVec3TransformCoord(&lookAt, &lookAt, &rotationMatrix);
	D3DXVec3TransformCoord(&up, &up, &rotationMatrix);
	lookAt = position + lookAt;
	D3DXMatrixLookAtLH(&mViewMatrix, &position, &lookAt, &up);

	RebuildFrustum();
}

void Camera::RebuildFrustum()
{
	float zMinimum, ratio;
	D3DXMATRIX frustumMatrix;

	zMinimum = -mProjectionMatrix._43 / mProjectionMatrix._33;
	ratio = mScreenSettings.Far / (mScreenSettings.Far - zMinimum);
	mProjectionMatrix._33 = ratio;
	mProjectionMatrix._43 = -ratio * zMinimum;

	D3DXMatrixMultiply(&frustumMatrix, &mViewMatrix, &mProjectionMatrix);

	//near
	mFrustumPlanes[0].a = frustumMatrix._14 + frustumMatrix._13;
	mFrustumPlanes[0].b = frustumMatrix._24 + frustumMatrix._23;
	mFrustumPlanes[0].c = frustumMatrix._34 + frustumMatrix._33;
	mFrustumPlanes[0].d = frustumMatrix._44 + frustumMatrix._43;
	D3DXPlaneNormalize(&mFrustumPlanes[0], &mFrustumPlanes[0]);

	//far
	mFrustumPlanes[1].a = frustumMatrix._14 - frustumMatrix._13;
	mFrustumPlanes[1].b = frustumMatrix._24 - frustumMatrix._23;
	mFrustumPlanes[1].c = frustumMatrix._34 - frustumMatrix._33;
	mFrustumPlanes[1].d = frustumMatrix._44 - frustumMatrix._43;
	D3DXPlaneNormalize(&mFrustumPlanes[1], &mFrustumPlanes[1]);

	//left
	mFrustumPlanes[2].a = frustumMatrix._14 + frustumMatrix._11;
	mFrustumPlanes[2].b = frustumMatrix._24 + frustumMatrix._21;
	mFrustumPlanes[2].c = frustumMatrix._34 + frustumMatrix._31;
	mFrustumPlanes[2].d = frustumMatrix._44 + frustumMatrix._41;
	D3DXPlaneNormalize(&mFrustumPlanes[2], &mFrustumPlanes[2]);

	//right
	mFrustumPlanes[3].a = frustumMatrix._14 - frustumMatrix._11;
	mFrustumPlanes[3].b = frustumMatrix._24 - frustumMatrix._21;
	mFrustumPlanes[3].c = frustumMatrix._34 - frustumMatrix._31;
	mFrustumPlanes[3].d = frustumMatrix._44 - frustumMatrix._41;
	D3DXPlaneNormalize(&mFrustumPlanes[3], &mFrustumPlanes[3]);

	//top
	mFrustumPlanes[4].a = frustumMatrix._14 - frustumMatrix._12;
	mFrustumPlanes[4].b = frustumMatrix._24 - frustumMatrix._22;
	mFrustumPlanes[4].c = frustumMatrix._34 - frustumMatrix._32;
	mFrustumPlanes[4].d = frustumMatrix._44 - frustumMatrix._42;
	D3DXPlaneNormalize(&mFrustumPlanes[4], &mFrustumPlanes[4]);

	//bottom
	mFrustumPlanes[5].a = frustumMatrix._14 + frustumMatrix._12;
	mFrustumPlanes[5].b = frustumMatrix._24 + frustumMatrix._22;
	mFrustumPlanes[5].c = frustumMatrix._34 + frustumMatrix._32;
	mFrustumPlanes[5].d = frustumMatrix._44 + frustumMatrix._42;
	D3DXPlaneNormalize(&mFrustumPlanes[5], &mFrustumPlanes[5]);
}

bool Camera::IsBoxInView(Box box)
{
	Vector3 center = box.GetBoundCenter();
	Vector3 size = box.GetSize();

	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X - size.X), (center.Y - size.Y), (center.Z - size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X + size.X), (center.Y - size.Y), (center.Z - size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X - size.X), (center.Y + size.Y), (center.Z - size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X - size.X), (center.Y - size.Y), (center.Z + size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X + size.X), (center.Y + size.Y), (center.Z - size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X + size.X), (center.Y - size.Y), (center.Z + size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X - size.X), (center.Y + size.Y), (center.Z + size.Z))) >= 0.0f
			|| D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3((center.X + size.X), (center.Y + size.Y), (center.Z + size.Z))) >= 0.0f)
		{
			continue;
		}
		return false;
	}
	return true;
}

bool Camera::IsSphereInView(float xCenter, float yCenter, float zCenter, float radius)
{
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&mFrustumPlanes[i], &D3DXVECTOR3(xCenter, yCenter, zCenter)) < -radius)
		{
			return false;
		}
	}
	return true;
}