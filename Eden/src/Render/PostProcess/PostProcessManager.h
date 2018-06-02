#pragma once
#include "Render/Graphics/GraphicsManager.h"

class PostProcessManager
{
public:
    PostProcessManager(GraphicsManager *graphicsManager);
    ~PostProcessManager();

    void ApplyToneMappingAndBloom();

private:
    GraphicsManager *mGraphicsManager;
};