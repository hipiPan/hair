#include "hair_asset.h"
#include "hair_instance.h"
#include "rhi/ez_vulkan.h"

int HairAsset::add_vertex()
{
    _positions.push_back(glm::vec3(0.0f));
    return _num_vertices++;
}

void HairAsset::set_vertex_position(int id, const glm::vec3& pos)
{
    _positions[id] = pos;
}

int HairAsset::add_strand()
{
    _vertex_counts.push_back(0);
    return _num_strands++;
}

void HairAsset::set_stand_vertex_count(int id, int count)
{
    _vertex_counts[id] = count;
}

void HairAsset::finalize_build()
{
    // Build strips
    for (int i = 0, segment_base = 0; i < _num_strands; ++i, segment_base += 2)
    {
        int strand_particle_count = _vertex_counts[i];
        int strip_vertex_count = strand_particle_count * 2;
        int strip_segment_count = strand_particle_count - 1;
        int strip_triangle_count = strip_segment_count * 2;
        int strip_index_count = strip_triangle_count * 3;
        for (int j = 0; j < strip_segment_count; ++j, segment_base += 2)
        {
            // First Triangle
            _indices.push_back(segment_base + 0);
            _indices.push_back(segment_base + 3);
            _indices.push_back(segment_base + 1);

            // Second Triangle
            _indices.push_back(segment_base + 0);
            _indices.push_back(segment_base + 2);
            _indices.push_back(segment_base + 3);
        }
    }
}

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
    instance->position_buffer = create_rw_buffer(_positions.data(), _positions.size() * sizeof(glm::vec3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    instance->index_buffer = create_rw_buffer(_indices.data(), _indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    return instance;
}