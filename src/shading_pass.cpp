#include "shading_pass.h"
#include "renderer.h"
#include <rhi/shader_manager.h>

ShadingPass::ShadingPass(Renderer* renderer)
{
    _renderer = renderer;
}

ShadingPass::~ShadingPass()
{
}

void ShadingPass::render()
{
    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[2];
    rt_barriers[0] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_RENDERTARGET);
    rt_barriers[1] = ez_image_barrier(_renderer->_depth_rt, EZ_RESOURCE_STATE_DEPTH_WRITE);
    ez_pipeline_barrier(0, 0, nullptr, 2, rt_barriers);

    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _renderer->_color_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};

    EzRenderingAttachmentInfo depth_info{};
    depth_info.texture = _renderer->_depth_rt;
    depth_info.clear_value.depthStencil = {1.0f, 1};

    EzRenderingInfo rendering_info{};
    rendering_info.width = _renderer->_width;
    rendering_info.height = _renderer->_height;
    rendering_info.colors.push_back(color_info);
    rendering_info.depth.push_back(depth_info);
    ez_begin_rendering(rendering_info);
    // TODO
    ez_end_rendering();
}