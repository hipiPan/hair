#include "hair_asset.h"
#include "hair_instance.h"
#include "rhi/ez_vulkan.h"

int HairAsset::add_vertex()
{
    _next_vertex_id++;
    _positions.push_back(glm::vec3(0.0f));
    return _next_vertex_id;
}

void HairAsset::set_vertex_position(int id, const glm::vec3& pos)
{
    _positions[id] = pos;
}

int HairAsset::add_strand()
{
    _next_strand_id++;
    _vertex_counts.push_back(0);
    return _next_strand_id;
}

void HairAsset::set_stand_vertex_count(int id, int count)
{
    _vertex_counts[id] = count;
}

HairInstance* HairAsset::create_instance()
{
    return nullptr;
}