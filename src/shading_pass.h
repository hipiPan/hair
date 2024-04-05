#pragma once
#include <rhi/ez_vulkan.h>
#include <glm/glm.hpp>

class Renderer;

struct HairRenderData
{
    glm::vec4 layer_depths;
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

    glm::vec4 compute_deep_shadow_layer_depths(float layer_distribution);

    void update_render_data_buffer(HairRenderData data);

private:
    Renderer* _renderer;
    EzTexture _front_shadow_rt = VK_NULL_HANDLE;
    EzTexture _deep_shadow_layers_rt = VK_NULL_HANDLE;
    EzBuffer _render_data_buffer = VK_NULL_HANDLE;
    EzSampler _sampler = VK_NULL_HANDLE;
};