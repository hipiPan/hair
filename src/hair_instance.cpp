#include "hair_instance.h"

HairInstance::StrandGroup::~StrandGroup()
{
    if (index_buffer)
        ez_destroy_buffer(index_buffer);
    if (position_buffer)
        ez_destroy_buffer(position_buffer);
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