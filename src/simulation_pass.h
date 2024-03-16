#pragma once
#include <rhi/ez_vulkan.h>
#include <glm/glm.hpp>

class Renderer;

struct SolverData
{
    glm::vec4 world_gravity;
    float dt;
    float damping;
    float damping_interval;
};

class SimulationPass
{
public:
    SimulationPass(Renderer* renderer);

    ~SimulationPass();

    void execute(float dt);

    float get_simulation_timestep();

private:
    void initialize();

    void solver(int step_count);

    void create_solver_data_buffer();

    void update_solver_data_buffer();

private:
    Renderer* _renderer;
    bool _initialized = false;
    float _accumulation_time = 0.0f;
    SolverData _solver_data;
    EzBuffer _solver_data_buffer = VK_NULL_HANDLE;
};