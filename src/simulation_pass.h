#pragma once
#include <rhi/ez_vulkan.h>

class Renderer;

class SimulationPass
{
public:
    SimulationPass(Renderer* renderer);

    ~SimulationPass();

    void execute(float dt);

    float get_simulation_timestep();

private:
    void initialize();

private:
    Renderer* _renderer;
    bool _initialized = false;
    float _accumulation_time = 0.0f;
};