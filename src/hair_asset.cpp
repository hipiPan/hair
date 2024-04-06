#include "hair_asset.h"
#include "hair_instance.h"
#include "rhi/ez_vulkan.h"
#include <unordered_map>

int HairAsset::Builder::add_vertex()
{
    _positions.push_back(glm::vec3(0.0f));
    return _num_vertices++;
}

int HairAsset::Builder::get_num_vertices()
{
    return _num_vertices;
}

void HairAsset::Builder::set_vertex_position(int id, const glm::vec3& pos)
{
    _positions[id] = pos;
}

int HairAsset::Builder::add_strand()
{
    _vertex_counts.push_back(0);
    _vertex_offsets.push_back(0);
    return _num_strands++;
}

int HairAsset::Builder::get_num_strands()
{
    return _num_strands;
}

void HairAsset::Builder::set_stand_vertex_count(int id, int count)
{
    _vertex_counts[id] = count;
}

void HairAsset::Builder::set_stand_vertex_offset(int id, int offset)
{
    _vertex_offsets[id] = offset;
}

HairAsset* HairAsset::Builder::build()
{
    HairAsset* asset = new HairAsset();

    // Build strand groups
    int vertex_count = 0;
    int vertex_offset = 0;
    StrandGroup* strand_group = nullptr;
    std::unordered_map<int, StrandGroup*> strand_group_dict;
    for (int i = 0; i < _num_strands; i++)
    {
        vertex_count = _vertex_counts[i];
        vertex_offset = _vertex_offsets[i];
        if (strand_group_dict.find(vertex_count) == strand_group_dict.end())
        {
            strand_group = new StrandGroup();
            strand_group_dict[vertex_count] = strand_group;
        }

        float strand_length = 0.0f;
        strand_group->strand_count++;
        strand_group->strand_particle_count = vertex_count;
        strand_group->root_positions.push_back(glm::vec4(_positions[vertex_offset], 0.0f));
        strand_group->root_scales.push_back(1.0f);
        for (int j = vertex_offset; j < vertex_offset + vertex_count; ++j)
        {
            if (j > vertex_offset)
            {
                strand_length += glm::distance(_positions[j - 1], _positions[j]);
            }
            strand_group->positions.push_back(glm::vec4(_positions[j], 0.0f));
        }

        if (strand_length >= strand_group->max_strand_length)
            strand_group->max_strand_length = strand_length;
        strand_group->lengths.push_back(strand_length);
    }

    for (auto iter : strand_group_dict)
    {
        strand_group = iter.second;

        strand_group->max_strand_length_interval = strand_group->max_strand_length / (strand_group->strand_particle_count - 1);

        // Build Scales
        for (int i = 0; i < strand_group->strand_count; ++i)
        {
            strand_group->root_scales[i] = strand_group->lengths[i] / strand_group->max_strand_length;
        }

        // Build indices
        int strand_particle_count = strand_group->strand_particle_count;
        int strip_vertex_count = strand_particle_count * 2;
        int strip_segment_count = strand_particle_count - 1;
        int strip_triangle_count = strip_segment_count * 2;
        int strip_index_count = strip_triangle_count * 3;
        for (int i = 0, segment_base = 0; i < strand_group->strand_count; ++i, segment_base += 2)
        {
            for (int j = 0; j < strip_segment_count; ++j, segment_base += 2)
            {
                // First Triangle
                strand_group->indices.push_back(segment_base + 0);
                strand_group->indices.push_back(segment_base + 3);
                strand_group->indices.push_back(segment_base + 1);

                // Second Triangle
                strand_group->indices.push_back(segment_base + 0);
                strand_group->indices.push_back(segment_base + 2);
                strand_group->indices.push_back(segment_base + 3);
            }
        }

        asset->_strand_groups.push_back(strand_group);
    }

    return asset;
}

HairAsset::HairAsset()
{}

HairAsset::~HairAsset()
{}

EzBuffer create_rw_buffer(void* data, uint32_t data_size, VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
{
    EzBuffer buffer;
    EzBufferDesc buffer_desc = {};
    buffer_desc.size = data_size;
    buffer_desc.usage = usage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, buffer);

    VkBufferMemoryBarrier2 barrier;
    if (data)
    {
        barrier = ez_buffer_barrier(buffer, EZ_RESOURCE_STATE_COPY_DEST);
        ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);
        ez_update_buffer(buffer, data_size, 0, data);
    }

    EzResourceState flag = EZ_RESOURCE_STATE_UNDEFINED;
    if ((usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) != 0)
        flag |= EZ_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if ((usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) != 0)
        flag |= EZ_RESOURCE_STATE_INDEX_BUFFER;
    if ((usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) != 0)
        flag |= EZ_RESOURCE_STATE_INDIRECT_ARGUMENT;
    barrier = ez_buffer_barrier(buffer, EZ_RESOURCE_STATE_SHADER_RESOURCE | EZ_RESOURCE_STATE_UNORDERED_ACCESS | flag);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    return buffer;
}

HairInstance* HairAsset::create_instance()
{
    HairInstance* instance = new HairInstance();
    for (auto strand_group : _strand_groups)
    {
        HairInstance::StrandGroup* strand_group_ins = new HairInstance::StrandGroup();
        strand_group_ins->constant.strand_count = strand_group->strand_count;
        strand_group_ins->constant.strand_particle_count = strand_group->strand_particle_count;
        strand_group_ins->constant.strand_particle_stride = 1;
        strand_group_ins->constant.max_strand_length_interval = strand_group->max_strand_length_interval;
        strand_group_ins->constant.particle_diameter = 0.0006f;
        strand_group_ins->index_count = strand_group->indices.size();
        strand_group_ins->root_position_buffer = create_rw_buffer(strand_group->root_positions.data(), strand_group->root_positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->root_scale_buffer = create_rw_buffer(strand_group->root_scales.data(), strand_group->root_scales.size() * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->position_buffer = create_rw_buffer(strand_group->positions.data(), strand_group->positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->position_pre_buffer = create_rw_buffer(nullptr, strand_group->positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->position_pre_pre_buffer = create_rw_buffer(nullptr, strand_group->positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->position_corr_buffer = create_rw_buffer(nullptr, strand_group->positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->velocity_buffer = create_rw_buffer(nullptr, strand_group->positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->velocity_pre_buffer = create_rw_buffer(nullptr, strand_group->positions.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        strand_group_ins->index_buffer = create_rw_buffer(strand_group->indices.data(), strand_group->indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        instance->strand_groups.push_back(strand_group_ins);
    }
    return instance;
}