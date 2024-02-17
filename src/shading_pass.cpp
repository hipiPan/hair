#include "shading_pass.h"
#include "renderer.h"
#include "hair_instance.h"
#include <rhi/shader_manager.h>

ShadingPass::ShadingPass(Renderer* renderer)
{
    _renderer = renderer;
}

ShadingPass::~ShadingPass()
{
}

void ShadingPass::execute()
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

    ez_set_viewport(0, 0, (float)_renderer->_width, (float)_renderer->_height);
    ez_set_scissor(0, 0, (int32_t)_renderer->_width, (int32_t)_renderer->_height);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("shader://hair_shading.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("shader://hair_shading.frag"));

    HairInstance* hair_instance = _renderer->_hair_instance;
    HairConstantBlock hair_constant{};
    hair_constant.particle_diameter = 0.02f;
    for (auto strand_group : hair_instance->strand_groups)
    {
        hair_constant.strand_particle_stride = 1;
        hair_constant.strand_particle_count = strand_group->strand_particle_count;
        ez_push_constants(&hair_constant, sizeof(HairConstantBlock), 0);

        ez_bind_buffer(0, _renderer->_view_buffer, _renderer->_view_buffer->size);
        ez_bind_buffer(1, strand_group->position_buffer, strand_group->position_buffer->size);
        ez_bind_index_buffer(strand_group->index_buffer, VK_INDEX_TYPE_UINT32);
        ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        ez_draw_indexed(strand_group->index_count, 0, 0);
    }

    ez_end_rendering();
}