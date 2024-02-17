#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hair_shader_defs.glsl"

layout(std140, binding = 0) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
} view_buffer;

struct Vertex
{
    float x, y, z;
};

layout(std430, binding = 1) restrict readonly buffer VertexDataBufferBlock
{
    Vertex data[];
} vertex_data_buffer;

layout(push_constant) uniform ConstantBlock
{
    uint strand_count;
    uint strand_particle_count;
    uint strand_particle_stride;
    float particle_diameter;
} constant;

vec3 load_position(uint i)
{
    return vec3(vertex_data_buffer.data[i].x, vertex_data_buffer.data[i].y, vertex_data_buffer.data[i].z);
}

struct HairVertexWS
{
    vec3 position_ws;
};

HairVertexWS get_hair_vertex_ws()
{
    uint particle_index = gl_VertexIndex / 2;
    uint strand_index = particle_index / constant.strand_particle_count;
    uint strand_particle_begin = strand_index * constant.strand_particle_count;
    uint strand_particle_end = strand_particle_begin + constant.strand_particle_count;
    uint vertex_index = particle_index % constant.strand_particle_count;

    uint i = strand_particle_begin + vertex_index;
    uint i_next = i + constant.strand_particle_stride;
    uint i_prev = i - constant.strand_particle_stride;
    uint i_head = strand_particle_begin;
    uint i_tail = strand_particle_end - constant.strand_particle_stride;

    vec3 p = load_position(i);
    vec3 r0 = (i == i_head) ? load_position(i_next) - p : p - load_position(i_prev);
    vec3 r1 = (i == i_tail) ? r0 : load_position(i_next) - p;

    vec3 vertex_bitangent_ws = (r0 + r1);
    vec3 t = normalize(view_buffer.view_position.xyz - p);
    vec3 vertex_tangent_ws = normalize_safe(cross(t, vertex_bitangent_ws));
    vec3 vertex_normal_ws = normalize_safe(cross(vertex_bitangent_ws, vertex_tangent_ws));

    float angle = (0.5 - (gl_VertexIndex % 2) * 0.5) * 6.2831853;
    vec2 vertex_offset_2d = vec2(cos(angle), sin(angle));
    float radius = 0.5 * constant.particle_diameter;
    vec3 vertex_offset_ws = (radius * vertex_offset_2d.x * vertex_tangent_ws) + (radius * vertex_offset_2d.y * vertex_normal_ws);
    vec3 vertex_position_ws = p + vertex_offset_ws;

    HairVertexWS v;
    v.position_ws = vertex_position_ws;
    return v;
}

void main() 
{
    HairVertexWS v = get_hair_vertex_ws();
    gl_Position = view_buffer.proj_matrix * view_buffer.view_matrix * vec4(v.position_ws, 1.0);
}