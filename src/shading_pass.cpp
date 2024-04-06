#include "shading_pass.h"
#include "renderer.h"
#include "hair_instance.h"
#include "camera.h"
#include "global_defines.h"
#include <rhi/shader_manager.h>

ShadingPass::ShadingPass(Renderer* renderer)
{
    _renderer = renderer;

    EzBufferDesc buffer_desc{};
    buffer_desc.size = sizeof(HairRenderData);
    buffer_desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, _render_data_buffer);

    EzTextureDesc desc{};
    desc.width = (uint32_t)SHADOW_RESOLUTION;
    desc.height = (uint32_t)SHADOW_RESOLUTION;
    desc.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    ez_create_texture(desc, _deep_shadow_layers_rt);
    ez_create_texture_view(_deep_shadow_layers_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

    desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ez_create_texture(desc, _front_shadow_rt);
    ez_create_texture_view(_front_shadow_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1);

    EzSamplerDesc sampler_desc{};
    sampler_desc.mag_filter = VK_FILTER_NEAREST;
    sampler_desc.min_filter = VK_FILTER_NEAREST;
    sampler_desc.address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ez_create_sampler(sampler_desc, _sampler);
}

ShadingPass::~ShadingPass()
{
    ez_destroy_buffer(_render_data_buffer);
    ez_destroy_texture(_front_shadow_rt);
    ez_destroy_texture(_deep_shadow_layers_rt);
    ez_destroy_sampler(_sampler);
}

glm::vec4 ShadingPass::compute_deep_shadow_layer_depths(float layer_distribution)
{
    const float exponent = glm::clamp(layer_distribution, 0.f, 1.f) * 5.2f + 1;
    glm::vec4 depths;
    depths.x = glm::pow(0.2f, exponent);
    depths.y = glm::pow(0.4f, exponent);
    depths.z = glm::pow(0.6f, exponent);
    depths.w = glm::pow(0.8f, exponent);
    return depths;
}

void ShadingPass::update_render_data_buffer(HairRenderData data)
{
    VkBufferMemoryBarrier2 barrier = ez_buffer_barrier(_render_data_buffer, EZ_RESOURCE_STATE_COPY_DEST);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    ez_update_buffer(_render_data_buffer, sizeof(HairRenderData), 0, &data);

    barrier = ez_buffer_barrier(_render_data_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);
}

void ShadingPass::execute()
{
    HairInstance* hair_instance = _renderer->_hair_instance;
    for (auto strand_group : hair_instance->strand_groups)
    {
        strand_group->sync_buffers();
    }

    front_shadow_pass();

    deep_opacity_map_pass();

    main_pass();
}

void ShadingPass::front_shadow_pass()
{
    HairInstance* hair_instance = _renderer->_hair_instance;

    HairRenderData shadow_render_data{};
    float ortho_height = glm::max(_renderer->shadow_max_extents.y - _renderer->shadow_min_extents.y, 0.0f);
    shadow_render_data.radius_at_depth1 = 0.5f * ortho_height / SHADOW_RESOLUTION;
    update_render_data_buffer(shadow_render_data);

    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[1];
    rt_barriers[0] = ez_image_barrier(_front_shadow_rt, EZ_RESOURCE_STATE_DEPTH_WRITE);
    ez_pipeline_barrier(0, 0, nullptr, 1, rt_barriers);

    EzRenderingAttachmentInfo depth_info{};
    depth_info.texture = _front_shadow_rt;
    depth_info.clear_value.depthStencil = {1.0f, 1};

    EzRenderingInfo rendering_info{};
    rendering_info.width = SHADOW_RESOLUTION;
    rendering_info.height = SHADOW_RESOLUTION;
    rendering_info.depth.push_back(depth_info);
    ez_begin_rendering(rendering_info);

    ez_set_viewport(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);
    ez_set_scissor(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("shader://hair_shading.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("shader://hair_shadow.frag"));

    for (auto strand_group : hair_instance->strand_groups)
    {
        ez_push_constants(&strand_group->constant, sizeof(HairConstant), 0);
        ez_bind_buffer(0, _renderer->_shadow_view_buffer);
        ez_bind_buffer(1, _render_data_buffer);
        ez_bind_buffer(2, strand_group->position_buffer);
        ez_bind_index_buffer(strand_group->index_buffer, VK_INDEX_TYPE_UINT32);
        ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        ez_draw_indexed(strand_group->index_count, 0, 0);
    }

    ez_end_rendering();
}

void ShadingPass::deep_opacity_map_pass()
{
    HairInstance* hair_instance = _renderer->_hair_instance;

    HairRenderData shadow_render_data{};
    float ortho_height = glm::max(_renderer->shadow_max_extents.y - _renderer->shadow_min_extents.y, 0.0f);
    shadow_render_data.radius_at_depth1 = 0.5f * ortho_height / SHADOW_RESOLUTION;
    shadow_render_data.layer_depths = compute_deep_shadow_layer_depths(1.0f);
    update_render_data_buffer(shadow_render_data);

    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 barriers[2];
    barriers[0] = ez_image_barrier(_deep_shadow_layers_rt, EZ_RESOURCE_STATE_RENDERTARGET);
    barriers[1] = ez_image_barrier(_front_shadow_rt, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    ez_pipeline_barrier(0, 0, nullptr, 2, barriers);

    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _deep_shadow_layers_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 0.0f};

    EzRenderingInfo rendering_info{};
    rendering_info.width = SHADOW_RESOLUTION;
    rendering_info.height = SHADOW_RESOLUTION;
    rendering_info.colors.push_back(color_info);
    ez_begin_rendering(rendering_info);

    ez_set_viewport(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);
    ez_set_scissor(0, 0, SHADOW_RESOLUTION, SHADOW_RESOLUTION);

    EzDepthState depth_state{};
    depth_state.depth_test = false;
    depth_state.depth_write = false;
    ez_set_depth_state(depth_state);

    EzBlendState blend_state{};
    blend_state.blend_enable = true;
    blend_state.src_color = VK_BLEND_FACTOR_ONE;
    blend_state.dst_color = VK_BLEND_FACTOR_ONE;
    blend_state.src_alpha = VK_BLEND_FACTOR_ONE;
    blend_state.dst_alpha = VK_BLEND_FACTOR_ONE;
    ez_set_blend_state(blend_state);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("shader://hair_shading.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("shader://hair_dom_shadow.frag"));

    for (auto strand_group : hair_instance->strand_groups)
    {
        ez_push_constants(&strand_group->constant, sizeof(HairConstant), 0);
        ez_bind_buffer(0, _renderer->_shadow_view_buffer);
        ez_bind_buffer(1, _render_data_buffer);
        ez_bind_buffer(2, strand_group->position_buffer);
        ez_bind_texture(3, _front_shadow_rt, 0);
        ez_bind_sampler(5, _sampler);
        ez_bind_index_buffer(strand_group->index_buffer, VK_INDEX_TYPE_UINT32);
        ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        ez_draw_indexed(strand_group->index_count, 0, 0);
    }

    ez_end_rendering();
}

void ShadingPass::main_pass()
{
    HairInstance* hair_instance = _renderer->_hair_instance;

    HairRenderData main_render_data{};
    main_render_data.radius_at_depth1 = 0.5f * glm::tan(_renderer->_camera->get_fov() * 0.5f) / (0.5f * _renderer->_height);
    main_render_data.layer_depths = compute_deep_shadow_layer_depths(1.0f);
    update_render_data_buffer(main_render_data);

    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 barriers[5];
    barriers[0] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_RENDERTARGET);
    barriers[1] = ez_image_barrier(_renderer->_depth_rt, EZ_RESOURCE_STATE_DEPTH_WRITE);
    barriers[2] = ez_image_barrier(_renderer->_resolve_rt, EZ_RESOURCE_STATE_RENDERTARGET);
    barriers[3] = ez_image_barrier(_front_shadow_rt, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    barriers[4] = ez_image_barrier(_deep_shadow_layers_rt, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    ez_pipeline_barrier(0, 0, nullptr, 5, barriers);

    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _renderer->_color_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
    color_info.resolve_texture = _renderer->_resolve_rt;
    color_info.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;

    EzRenderingAttachmentInfo depth_info{};
    depth_info.texture = _renderer->_depth_rt;
    depth_info.clear_value.depthStencil = {1.0f, 1};

    EzRenderingInfo rendering_info{};
    rendering_info.width = _renderer->_width;
    rendering_info.height = _renderer->_height;
    rendering_info.colors.push_back(color_info);
    rendering_info.depth.push_back(depth_info);
    ez_begin_rendering(rendering_info);

    EzMultisampleState ms{};
    ms.sample_shading = true;
    ms.samples = VK_SAMPLE_COUNT_4_BIT;
    ez_set_multisample_state(ms);

    ez_set_viewport(0, 0, (float)_renderer->_width, (float)_renderer->_height);
    ez_set_scissor(0, 0, (int32_t)_renderer->_width, (int32_t)_renderer->_height);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("shader://hair_shading.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("shader://hair_shading.frag"));

    for (auto strand_group : hair_instance->strand_groups)
    {
        ez_push_constants(&strand_group->constant, sizeof(HairConstant), 0);
        ez_bind_buffer(0, _renderer->_view_buffer);
        ez_bind_buffer(1, _render_data_buffer);
        ez_bind_buffer(2, strand_group->position_buffer);
        ez_bind_texture(3, _front_shadow_rt, 0);
        ez_bind_texture(4, _deep_shadow_layers_rt, 0);
        ez_bind_sampler(5, _sampler);
        ez_bind_index_buffer(strand_group->index_buffer, VK_INDEX_TYPE_UINT32);
        ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        ez_draw_indexed(strand_group->index_count, 0, 0);
    }

    ez_end_rendering();
}