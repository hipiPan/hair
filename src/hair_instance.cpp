#include "hair_instance.h"

HairInstance::HairInstance()
{}

HairInstance::~HairInstance()
{
    if (index_buffer)
        ez_destroy_buffer(index_buffer);
    if (position_buffer)
        ez_destroy_buffer(position_buffer);
}