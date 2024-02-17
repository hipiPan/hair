#pragma once
#include <rhi/ez_vulkan.h>

struct HairConstantBlock
{
    uint32_t strand_count;
    uint32_t strand_particle_count;
    uint32_t strand_particle_stride;
    float particle_diameter;
};

class HairInstance
{
public:
    HairInstance();
    ~HairInstance();

    class StrandGroup
    {
    public:
        StrandGroup() = default;
        ~StrandGroup();

        int strand_count = 0;
        int strand_particle_count = 0;
        int index_count = 0;
        EzBuffer index_buffer = VK_NULL_HANDLE;
        EzBuffer root_position_buffer = VK_NULL_HANDLE;
    };

    std::vector<StrandGroup*> strand_groups;
};