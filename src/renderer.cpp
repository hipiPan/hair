#include "renderer.h"
#include "camera.h"
#include "hair_instance.h"
#include "simulation_pass.h"
#include "shading_pass.h"
#include "global_defines.h"

Renderer::Renderer()
{
    EzBufferDesc buffer_desc{};
    buffer_desc.size = sizeof(ViewBufferType);
    buffer_desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, _view_buffer);
    ez_create_buffer(buffer_desc, _shadow_view_buffer);

    _simulation_pass = new SimulationPass(this);
    _shading_pass = new ShadingPass(this);
}

Renderer::~Renderer()
{
    delete _shading_pass;
    delete _simulation_pass;

    if (_view_buffer)
        ez_destroy_buffer(_view_buffer);
    if (_shadow_view_buffer)
        ez_destroy_buffer(_shadow_view_buffer);
    if (_color_rt)
        ez_destroy_texture(_color_rt);
    if (_depth_rt)
        ez_destroy_texture(_depth_rt);
    if (_resolve_rt)
        ez_destroy_texture(_resolve_rt);
}

void Renderer::set_hair_instance(HairInstance* ins)
{
    _hair_instance = ins;
}

void Renderer::set_camera(Camera* camera)
{
    _camera = camera;
}

void Renderer::update_rendertarget()
{
    EzTextureDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

    if (_resolve_rt)
        ez_destroy_texture(_resolve_rt);
    ez_create_texture(desc, _resolve_rt);
    ez_create_texture_view(_resolve_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
    desc.samples = VK_SAMPLE_COUNT_4_BIT;

    if (_color_rt)
        ez_destroy_texture(_color_rt);
    ez_create_texture(desc, _color_rt);
    ez_create_texture_view(_color_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

    desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (_depth_rt)
        ez_destroy_texture(_depth_rt);
    ez_create_texture(desc, _depth_rt);
    ez_create_texture_view(_depth_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1);
}

void Renderer::update_view_buffer()
{
    glm::vec3 sun_direction = glm::vec3(0.3f, 0.5f, -1.0f);
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 5.0f);
    float radius = 5.0f;
    glm::vec3 max_extents = glm::vec3(radius);
    glm::vec3 min_extents = -max_extents;
    glm::mat4 light_view_matrix = glm::lookAt(center - sun_direction * -min_extents.z, center, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 light_model_matrix = glm::inverse(light_view_matrix);
    glm::mat4 light_ortho_matrix = glm::ortho(min_extents.x, max_extents.x, min_extents.y, max_extents.y, 0.0f, max_extents.z - min_extents.z);
    glm::mat4 sun_matrix = light_ortho_matrix * light_view_matrix;
    shadow_min_extents = min_extents;
    shadow_max_extents = max_extents;

    // Shadow view
    ViewBufferType shadow_view_buffer_type{};
    shadow_view_buffer_type.view_matrix = light_view_matrix;
    shadow_view_buffer_type.proj_matrix = light_ortho_matrix;
    shadow_view_buffer_type.sun_matrix = sun_matrix;
    shadow_view_buffer_type.view_position =  glm::vec4(glm::vec3(light_model_matrix[3][0], light_model_matrix[3][1], light_model_matrix[3][2]), 1.0f);
    shadow_view_buffer_type.sun_direction = glm::vec4(sun_direction, 1.0);

    // Main view
    glm::mat4 proj_matrix = _camera->get_proj_matrix();
    glm::mat4 view_matrix = _camera->get_view_matrix();
    ViewBufferType view_buffer_type{};
    view_buffer_type.view_matrix = view_matrix;
    view_buffer_type.proj_matrix = proj_matrix;
    view_buffer_type.sun_matrix = sun_matrix;
    view_buffer_type.view_position = glm::vec4(_camera->get_translation(), 1.0f);
    view_buffer_type.sun_direction = glm::vec4(sun_direction, 1.0);

    VkBufferMemoryBarrier2 barriers[2];
    barriers[0] = ez_buffer_barrier(_view_buffer, EZ_RESOURCE_STATE_COPY_DEST);
    barriers[1] = ez_buffer_barrier(_shadow_view_buffer, EZ_RESOURCE_STATE_COPY_DEST);
    ez_pipeline_barrier(0, 2, barriers, 0, nullptr);

    ez_update_buffer(_view_buffer, sizeof(ViewBufferType), 0, &view_buffer_type);
    ez_update_buffer(_shadow_view_buffer, sizeof(ViewBufferType), 0, &shadow_view_buffer_type);

    barriers[0] = ez_buffer_barrier(_view_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS);
    barriers[1] = ez_buffer_barrier(_shadow_view_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS);
    ez_pipeline_barrier(0, 2, barriers, 0, nullptr);
}

void Renderer::render(EzSwapchain swapchain, float dt)
{
    if (!_hair_instance || !_camera)
        return;

    if (swapchain->width == 0 || swapchain->height ==0)
        return;

    if (_width != swapchain->width || _height != swapchain->height)
    {
        _width = swapchain->width;
        _height = swapchain->height;
        update_rendertarget();
    }

    update_view_buffer();

    _simulation_pass->execute(dt);
    _shading_pass->execute();

    // Copy to swapchain
    EzTexture src_rt = _resolve_rt;
    VkImageMemoryBarrier2 copy_barriers[] = {
        ez_image_barrier(src_rt, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
        ez_image_barrier(swapchain, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
    };
    ez_pipeline_barrier(0, 0, nullptr, 2, copy_barriers);

    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.extent = { swapchain->width, swapchain->height, 1 };
    ez_copy_image(src_rt, swapchain, copy_region);

    _frame_number++;
}
