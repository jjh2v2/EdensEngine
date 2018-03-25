#include "Entity/SceneEntity.h"
#include "Render/DirectX/Context/Direct3DContext.h"
#include "Render/Material/Material.h"

SceneEntity::SceneEntity(Mesh *mesh, Material *material, ShadowMaterial *shadowMaterial /*= NULL*/)
{
	mMesh = mesh;
	mMaterial = material;
    mShadowMaterial = shadowMaterial;
	mPosition = Vector3(0, 0, 0);
	mRotation = Vector3(0, 0, 0);
	mScale = Vector3(1, 1, 1);
}

SceneEntity::~SceneEntity()
{

}

void SceneEntity::Update()
{
	
}

D3DXMATRIX SceneEntity::GetWorldMatrix(const Vector3 &position, const Vector3 &rotation, const Vector3 &scale)
{
    D3DXMATRIX modelMatrix;
    D3DXMATRIX positionMatrix;
    D3DXMATRIX rotationMatrix;
    D3DXMATRIX scalarMatrix;

    D3DXMatrixIdentity(&positionMatrix);
    D3DXMatrixIdentity(&rotationMatrix);
    D3DXMatrixIdentity(&scalarMatrix);
    D3DXMatrixIdentity(&modelMatrix);
    D3DXMatrixTranslation(&positionMatrix, position.X, position.Y, position.Z);
    D3DXMatrixRotationYawPitchRoll(&rotationMatrix, rotation.Y, rotation.X, rotation.Z);
    D3DXMatrixScaling(&scalarMatrix, scale.X, scale.Y, scale.Z);
    D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &scalarMatrix);
    D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &rotationMatrix);
    D3DXMatrixMultiply(&modelMatrix, &modelMatrix, &positionMatrix);
    D3DXMatrixTranspose(&modelMatrix, &modelMatrix);

    mCachedWorldMatrix = modelMatrix;

    return modelMatrix;
}

void SceneEntity::Render(RenderPassContext *renderPassContext)
{
    if (!mMesh->IsReady())
    {
        return;
    }

	GraphicsContext *graphicsContext = renderPassContext->GetGraphicsContext();

    D3DXMATRIX modelMatrix = GetWorldMatrix(mPosition, mRotation, mScale);
	mMaterial->GetMaterialBuffer()->SetWorldMatrix(modelMatrix);
	mMaterial->ApplyMaterial(renderPassContext);

	graphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	graphicsContext->SetVertexBuffer(0, mMesh->GetVertexBuffer());
	graphicsContext->SetIndexBuffer(mMesh->GetIndexBuffer());
	graphicsContext->Draw(mMesh->GetVertexCount());
}

void SceneEntity::RenderShadows(RenderPassContext *renderPassContext, const D3DXMATRIX &lightViewProjMatrix)
{
    if (!mMesh->IsReady() || !mShadowMaterial)
    {
        return;
    }

    GraphicsContext *graphicsContext = renderPassContext->GetGraphicsContext();

    D3DXMATRIX lightWorldViewProjMatrix = mCachedWorldMatrix * lightViewProjMatrix;             //use cached matrix because it should not change between passes
    mShadowMaterial->GetMaterialBuffer()->SetLightWorldViewProjMatrix(lightWorldViewProjMatrix);
    mShadowMaterial->ApplyMaterial(renderPassContext);

    graphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphicsContext->SetVertexBuffer(0, mMesh->GetVertexBuffer());
    graphicsContext->SetIndexBuffer(mMesh->GetIndexBuffer());
    graphicsContext->Draw(mMesh->GetVertexCount());
}