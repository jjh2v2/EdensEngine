#include "Render/Material/Material.h"

Material::Material(MaterialBuffer initialConstants, ConstantBuffer *constantBuffer)
{
	mConstants = initialConstants;
	mConstantBuffer = constantBuffer;
	CommitMaterialChanges();
}

Material::~Material()
{
	delete mConstantBuffer;
}

void Material::CommitMaterialChanges()
{
	mConstantBuffer->SetConstantBufferData(mConstantBuffer, sizeof(ConstantBuffer));
}