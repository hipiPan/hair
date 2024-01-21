#pragma once
#include <rhi/ez_vulkan.h>

class HairInstance
{
public:
    HairInstance();
    ~HairInstance();

    EzBuffer index_buffer = VK_NULL_HANDLE;
    EzBuffer position_buffer = VK_NULL_HANDLE;
};