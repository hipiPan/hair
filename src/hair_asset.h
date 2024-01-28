#pragma once
#include <vector>
#include <glm/glm.hpp>

class HairInstance;

class HairAsset
{
public:
    ~HairAsset();

    class StrandGroup
    {
    public:
        int strand_count = 0;
        int strand_particle_count = 0;
        std::vector<glm::vec3> positions;
        std::vector<uint32_t> indices;
    };

    class Builder
    {
    public:
        Builder() = default;
        ~Builder() = default;

        int add_vertex();
        int get_num_vertices();
        void set_vertex_position(int id, const glm::vec3& pos);

        int add_strand();
        void set_stand_vertex_count(int id, int count);
        void set_stand_vertex_offset(int id, int offset);

        HairAsset* build();

    private:
        friend class HairAsset;
        // Vertex attribute
        int _num_vertices = 0;
        std::vector<glm::vec3> _positions;

        // Strand attribute
        int _num_strands = 0;
        std::vector<int> _vertex_counts;
        std::vector<int> _vertex_offsets;
    };

    HairInstance* create_instance();

private:
    HairAsset();

    std::vector<StrandGroup*> _strand_groups;
};