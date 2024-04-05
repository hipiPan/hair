#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in highp vec3 in_position_ws;
layout(location = 1) in highp vec3 in_bitangent_ws;
layout(location = 2) in highp vec3 in_normal_ws;
layout(location = 3) in highp vec4 in_position_cs;
layout(location = 4) in highp float in_coverage;
layout(location = 0) out vec4 out_color;

layout(std140, binding = 1) uniform RenderDataBuffer
{
    vec4 layer_depths;
    float radius_at_depth1;
} render_data_buffer;

layout(binding = 3) uniform texture2D front_shadow_texture;
layout(binding = 4) uniform sampler shadow_sampler;

bool is_depth_closer(float a, float b)
{
    return a < b;
}

float compute_dom_weight(float distance_to_front_depth, float layer_depth)
{
    return is_depth_closer(distance_to_front_depth, layer_depth) ? 1.0 : 0.0;
}

vec4 compute_dom_weights(float distance_to_front_depth)
{
    vec4 weigths = vec4(0.0);
    weigths.x = compute_dom_weight(distance_to_front_depth, render_data_buffer.layer_depths.x);
    weigths.y = compute_dom_weight(distance_to_front_depth, render_data_buffer.layer_depths.y);
    weigths.z = compute_dom_weight(distance_to_front_depth, render_data_buffer.layer_depths.z);
    weigths.w = compute_dom_weight(distance_to_front_depth, render_data_buffer.layer_depths.w);
    return weigths;
}

void main()
{
    vec2 uv = (in_position_cs.xy + 1.0) / 2.0;
    float front_depth = texture(sampler2D(front_shadow_texture, shadow_sampler), uv).r;
    float distance_to_front_depth = abs(in_position_cs.z - front_depth);
    out_color = compute_dom_weights(distance_to_front_depth) * in_coverage;
}
