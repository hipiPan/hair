#pragma once
#include <rhi/ez_vulkan.h>
#include <glm/glm.hpp>

class Camera;
class HairInstance;

struct ViewBufferType
{
    glm::mat4 view_matrix;
    glm::mat4 proj_matrix;
    glm::mat4 pad0;
    glm::mat4 pad1;
};

class Renderer
{
public:
    Renderer();

    ~Renderer();

    void render(EzSwapchain swapchain);

    void set_hair_instance(HairInstance* ins);

    void set_camera(Camera* camera);

private:
    void update_rendertarget();

    void update_view_buffer();

    uint32_t _width = 0;
    uint32_t _height = 0;
    uint64_t _frame_number = 0;
    Camera* _camera;
    HairInstance* _hair_instance;
    EzBuffer _view_buffer = VK_NULL_HANDLE;
    EzTexture _color_rt = VK_NULL_HANDLE;
    EzTexture _depth_rt = VK_NULL_HANDLE;
    friend class ShadingPass;
    ShadingPass* _shading_pass = nullptr;
};