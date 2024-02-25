#include "simulation_pass.h"
#include "renderer.h"
#include "hair_instance.h"
#include <rhi/shader_manager.h>

#define PARTICLE_GROUP_SIZE 64

SimulationPass::SimulationPass(Renderer* renderer)
{
    _renderer = renderer;

    create_solver_data_buffer();
}

SimulationPass::~SimulationPass()
{
    ez_destroy_buffer(_solver_data_buffer);
}

float SimulationPass::get_simulation_timestep()
{
    // 1.0f / 30.0f
    return 0.0333f;
}

void SimulationPass::create_solver_data_buffer()
{
    EzBufferDesc buffer_desc{};
    buffer_desc.size = sizeof(SolverData);
    buffer_desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, _solver_data_buffer);
}

void SimulationPass::update_solver_data_buffer()
{
    VkBufferMemoryBarrier2 barrier = ez_buffer_barrier(_solver_data_buffer, EZ_RESOURCE_STATE_COPY_DEST);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    ez_update_buffer(_solver_data_buffer, sizeof(SolverData), 0, &_solver_data);

    barrier = ez_buffer_barrier(_solver_data_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);
}

void SimulationPass::initialize()
{

}

void SimulationPass::execute(float dt)
{
    float step_dt = get_simulation_timestep();
    _accumulation_time += dt;
    int step_count = (int)glm::floor(_accumulation_time / step_dt);
    _accumulation_time -= (float)step_count * step_dt;

    if (_accumulation_time < 0.0f)
        _accumulation_time = 0.0f;

    if (step_count <= 0)
        return;

    _solver_data.world_gravity = glm::vec4(0.0f, -9.8f, 0.0f, 1.0f);
    _solver_data.dt = step_dt;

    update_solver_data_buffer();

    if (!_initialized)
    {
        initialize();
        _initialized = true;
    }
}