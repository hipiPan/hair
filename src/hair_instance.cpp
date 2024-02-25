#include "hair_instance.h"

HairInstance::StrandGroup::~StrandGroup()
{
    if (index_buffer)
        ez_destroy_buffer(index_buffer);
    if (root_position_buffer)
        ez_destroy_buffer(root_position_buffer);
    if (position_buffer)
        ez_destroy_buffer(position_buffer);
    if (position_pre_buffer)
        ez_destroy_buffer(position_pre_buffer);
    if (position_pre_pre_buffer)
        ez_destroy_buffer(position_pre_pre_buffer);
    if (position_corr_buffer)
        ez_destroy_buffer(position_corr_buffer);
    if (velocity_buffer)
        ez_destroy_buffer(velocity_buffer);
    if (velocity_pre_buffer)
        ez_destroy_buffer(velocity_pre_buffer);
}

void swap_buffer(EzBuffer src, EzBuffer dst)
{
    EzBuffer temp = src;
    src = dst;
    dst = temp;
}

void HairInstance::StrandGroup::swap_buffers()
{
    VkBufferMemoryBarrier2 barriers[5];
    barriers[0] = ez_buffer_barrier(position_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS | EZ_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    barriers[1] = ez_buffer_barrier(position_pre_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS | EZ_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    barriers[2] = ez_buffer_barrier(position_pre_pre_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS | EZ_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    barriers[3] = ez_buffer_barrier(velocity_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS | EZ_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    barriers[4] = ez_buffer_barrier(velocity_pre_buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS | EZ_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    ez_pipeline_barrier(0, 5, barriers, 0, nullptr);

    swap_buffer(position_buffer, position_pre_buffer);
    swap_buffer(position_buffer, position_pre_pre_buffer);
    swap_buffer(velocity_buffer, velocity_pre_buffer);
}

HairInstance::HairInstance()
{}

HairInstance::~HairInstance()
{
    for (auto strand_group : strand_groups)
    {
        delete strand_group;
    }
    strand_groups.clear();
}