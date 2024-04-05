#pragma once
#include <rhi/ez_vulkan.h>

class Renderer;

struct HairRenderData
{
    float radius_at_depth1;
};

class ShadingPass
{
public:
    ShadingPass(Renderer* renderer);

    ~ShadingPass();

    void execute();

private:
    void front_shadow_pass();

    void deep_opacity_map_pass();

    void main_pass();

    void update_render_data_buffer(HairRenderData data);

private:
    Renderer* _renderer;
    EzTexture _front_shadow_rt = VK_NULL_HANDLE;
    EzTexture _deep_shadow_layers_rt = VK_NULL_HANDLE;
    EzBuffer _render_data_buffer = VK_NULL_HANDLE;
};