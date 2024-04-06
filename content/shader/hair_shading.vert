#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hair_shader_defs.glsl"

layout(std140, binding = 0) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    mat4 sun_matrix;
    vec4 view_position;
    vec4 sun_direction;
} view_buffer;

layout(std140, binding = 1) uniform RenderDataBuffer
{
    vec4 layer_depths;
    float radius_at_depth1;
} render_data_buffer;

struct Vertex
{
    float x, y, z, w;
};

layout(std430, binding = 2) restrict readonly buffer VertexDataBufferBlock
{
    Vertex data[];
} vertex_data_buffer;

layout(location = 0) out highp vec3 out_position_ws;
layout(location = 1) out highp vec3 out_bitangent_ws;
layout(location = 2) out highp vec3 out_normal_ws;
layout(location = 3) out highp vec4 out_position_cs;
layout(location = 4) out highp float out_coverage;

vec3 load_position(uint i)
{
    return vec3(vertex_data_buffer.data[i].x, vertex_data_buffer.data[i].y, vertex_data_buffer.data[i].z);
}

struct HairVertexWS
{
    vec3 position_ws;
    vec3 bitangent_ws;
    vec3 normal_ws;
    float coverage;
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

    float strand_radius = 0.5 * constant.particle_diameter;
    vec4 p_cs = view_buffer.proj_matrix * view_buffer.view_matrix * vec4(p, 1.0);
    float min_radius = p_cs.w * render_data_buffer.radius_at_depth1;
    float radius = max(min_radius, strand_radius);
    float coverage = strand_radius / radius;

    vec3 vertex_offset_ws = (radius * vertex_offset_2d.x * vertex_tangent_ws) + (radius * vertex_offset_2d.y * vertex_normal_ws);
    vec3 vertex_position_ws = p + vertex_offset_ws;

    HairVertexWS v;
    v.position_ws = vertex_position_ws;
    v.bitangent_ws = vertex_bitangent_ws;
    v.normal_ws = vertex_normal_ws;
    v.coverage = coverage;
    return v;
}

void main() 
{
    HairVertexWS v = get_hair_vertex_ws();
    out_position_ws = v.position_ws;
    out_bitangent_ws = v.bitangent_ws;
    out_normal_ws = v.normal_ws;
    out_coverage = v.coverage;
    out_position_cs = view_buffer.proj_matrix * view_buffer.view_matrix * vec4(v.position_ws, 1.0);
    gl_Position = out_position_cs;
}