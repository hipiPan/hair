#include "hair_asset.h"
#include "hair_instance.h"
#include "hair_importer.h"
#include "camera.h"
#include "camera_controller.h"
#include "renderer.h"
#include <core/path.h>
#include <input/input.h>
#include <rhi/ez_vulkan.h>
#include <rhi/shader_manager.h>
#include <rhi/shader_compiler.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static void window_size_callback(GLFWwindow* window, int w, int h)
{
}

static void cursor_position_callback(GLFWwindow* window, double pos_x, double pos_y)
{
    Input::get()->set_mouse_position((float)pos_x, (float)pos_y);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Input::get()->set_mouse_button(button, action);
}

static void mouse_scroll_callback(GLFWwindow* window, double offset_x, double offset_y)
{
    Input::get()->set_mouse_scroll((float)offset_y);
}

int main()
{
    Path::register_protocol("content", std::string(PROJECT_DIR) + "/content/");
    Path::register_protocol("hair", std::string(PROJECT_DIR) + "/content/hair/");
    Path::register_protocol("shader", std::string(PROJECT_DIR) + "/content/shader/");

    int init_width = 1024, init_height = 768;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfw_window = glfwCreateWindow(init_width, init_height, "hair", nullptr, nullptr);
    glfwSetWindowPos(glfw_window, 100, 100);
    glfwSetFramebufferSizeCallback(glfw_window, window_size_callback);
    glfwSetCursorPosCallback(glfw_window, cursor_position_callback);
    glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
    glfwSetScrollCallback(glfw_window, mouse_scroll_callback);

    ez_init();
    EzSwapchain swapchain = VK_NULL_HANDLE;
    ez_create_swapchain(glfwGetWin32Window(glfw_window), swapchain);
    ShaderManager::get()->setup();
    ShaderCompiler::get()->setup();

    HairAsset* hair_asset = load_hair_asset(Path::fix_path("hair://hair_02.abc"));
    HairInstance* hair_instance = hair_asset->create_instance();

    Camera* camera = new Camera();
    camera->set_aspect((float)init_width/(float)init_height);
    camera->set_translation(glm::vec3(0.0f, -2.0f, 18.0f));
    camera->set_euler(glm::vec3(0.0f, 0.0f, 0.0f));
    CameraController* camera_controller = new CameraController();
    camera_controller->set_camera(camera);

    Renderer* renderer = new Renderer();
    renderer->set_camera(camera);
    renderer->set_hair_instance(hair_instance);

    double time = 0.0;
    while (!glfwWindowShouldClose(glfw_window))
    {
        double current_time = glfwGetTime();
        float dt = time > 0.0 ? (float)(current_time - time) : (float)(1.0f / 60.0f);
        time = current_time;

        glfwPollEvents();

        EzSwapchainStatus swapchain_status = ez_update_swapchain(swapchain);

        if (swapchain_status == EzSwapchainStatus::NotReady)
            continue;

        if (swapchain_status == EzSwapchainStatus::Resized)
        {
            camera->set_aspect(swapchain->width/swapchain->height);
        }

        ez_acquire_next_image(swapchain);

        renderer->render(swapchain, dt);

        VkImageMemoryBarrier2 present_barrier[] = { ez_image_barrier(swapchain, EZ_RESOURCE_STATE_PRESENT) };
        ez_pipeline_barrier(0, 0, nullptr, 1, present_barrier);

        ez_present(swapchain);

        ez_submit();

        // Reset input
        Input::get()->reset();
    }

    delete renderer;
    delete camera;
    delete camera_controller;
    delete hair_asset;
    delete hair_instance;

    ez_destroy_swapchain(swapchain);
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    ShaderManager::get()->cleanup();
    ShaderCompiler::get()->cleanup();
    ez_flush();
    ez_terminate();
    return 0;
}