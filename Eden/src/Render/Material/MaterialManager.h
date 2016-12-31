#pragma once
#include "Render/Material/Material.h"

class MaterialManager
{
public:
	MaterialManager();
	~MaterialManager();

private:
	std::map<std::string, Material*> mMaterialLookup;
	DynamicArray<Material*> mMaterials;
};