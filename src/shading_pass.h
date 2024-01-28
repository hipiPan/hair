#pragma once
#include <rhi/ez_vulkan.h>

class Renderer;

class ShadingPass
{
public:
    ShadingPass(Renderer* renderer);

    ~ShadingPass();

    void render();

private:
    Renderer* _renderer;
};