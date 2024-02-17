#include "hair_instance.h"

HairInstance::StrandGroup::~StrandGroup()
{
    if (index_buffer)
        ez_destroy_buffer(index_buffer);
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