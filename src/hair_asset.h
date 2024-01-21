#pragma once
#include <vector>
#include <glm/glm.hpp>

class HairAsset
{
public:
    int add_vertex();
    void set_vertex_position(int id, const glm::vec3& pos);

    int add_strand();
    void set_stand_vertex_count(int id, int count);

private:
    // Vertex attribute
    int _next_vertex_id = -1;
    std::vector<glm::vec3> _positions;

    // Strand attribute
    int _next_strand_id = -1;
    std::vector<int> _vertex_counts;
};