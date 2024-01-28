#pragma once
#include <vector>
#include <glm/glm.hpp>

class HairInstance;

class HairAsset
{
public:
    int add_vertex();
    void set_vertex_position(int id, const glm::vec3& pos);

    int add_strand();
    void set_stand_vertex_count(int id, int count);

    void finalize_build();

    HairInstance* create_instance();

private:
    // Vertex attribute
    int _num_vertices = 0;
    std::vector<glm::vec3> _positions;

    // Strand attribute
    int _num_strands = 0;
    std::vector<int> _vertex_counts;

    std::vector<uint32_t> _indices;
};