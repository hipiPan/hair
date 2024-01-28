#pragma once
#include <rhi/ez_vulkan.h>

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
        EzBuffer index_buffer = VK_NULL_HANDLE;
        EzBuffer position_buffer = VK_NULL_HANDLE;
    };

    std::vector<StrandGroup*> strand_groups;
};