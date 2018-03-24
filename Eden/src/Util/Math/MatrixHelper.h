#pragma once
#include <d3d11.h>
#include <d3dx10math.h>
#include "Core/Vector/Vector3.h"

class MatrixHelper
{
public:
    static void CalculateFrustumExtentsD3DX(const D3DXMATRIXA16& cameraViewInv, const D3DXMATRIXA16 &cameraProj, float nearZ, float farZ,
        const D3DXMATRIXA16& lightViewProj, Vector3 &outMin, Vector3 &outMax)
    {
        float scaleXInv = 1.0f / cameraProj._11;
        float scaleYInv = 1.0f / cameraProj._22;

        D3DXMATRIXA16 cameraViewToLightProj = cameraViewInv * lightViewProj;

        D3DXVECTOR3 corners[8];
        float nearX = scaleXInv * nearZ;
        float nearY = scaleYInv * nearZ;
        float farX = scaleXInv * farZ;
        float farY = scaleYInv * farZ;

        corners[0] = D3DXVECTOR3(-nearX, nearY, nearZ);
        corners[1] = D3DXVECTOR3(nearX, nearY, nearZ);
        corners[2] = D3DXVECTOR3(-nearX, -nearY, nearZ);
        corners[3] = D3DXVECTOR3(nearX, -nearY, nearZ);
        corners[4] = D3DXVECTOR3(-farX, farY, farZ);
        corners[5] = D3DXVECTOR3(farX, farY, farZ);
        corners[6] = D3DXVECTOR3(-farX, -farY, farZ);
        corners[7] = D3DXVECTOR3(farX, -farY, farZ);

        D3DXVECTOR4 cornersLightView[8];
        D3DXVec3TransformArray(cornersLightView, sizeof(D3DXVECTOR4), corners, sizeof(D3DXVECTOR3), &cameraViewToLightProj, 8);

        D3DXVECTOR4 minCorner(cornersLightView[0]);
        D3DXVECTOR4 maxCorner(cornersLightView[0]);
        for (unsigned int i = 1; i < 8; ++i)
        {
            D3DXVec4Minimize(&minCorner, &minCorner, &cornersLightView[i]);
            D3DXVec4Maximize(&maxCorner, &maxCorner, &cornersLightView[i]);
        }

        outMin.X = minCorner.x;
        outMin.Y = minCorner.y;
        outMin.Z = minCorner.z;
        outMax.X = maxCorner.x;
        outMax.Y = maxCorner.y;
        outMax.Z = maxCorner.z;
    }
};