#include "simulation_pass.h"
#include "renderer.h"
#include "hair_instance.h"
#include <rhi/shader_manager.h>

#define PARTICLE_GROUP_SIZE 64

SimulationPass::SimulationPass(Renderer* renderer)
{
    _renderer = renderer;
}

SimulationPass::~SimulationPass()
{
}

float SimulationPass::get_simulation_timestep()
{
    // 1.0f / 30.0f
    return 0.0333f;
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

    if (!_initialized)
    {
        initialize();
        _initialized = true;
    }
}