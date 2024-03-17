#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in highp vec3 in_position_ws;
layout(location = 1) in highp vec3 in_tangent_ws;
layout(location = 2) in highp vec3 in_normal_ws;
layout(location = 0) out vec4 out_color;

layout(std140, binding = 0) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
} view_buffer;

void main()
{
    vec3 sun_direction = normalize(vec3(0.0, -1.0, -1.0));
    vec3 L = normalize(-sun_direction);
    vec3 V = normalize(view_buffer.view_position.xyz - in_position_ws);
    vec3 T = normalize(in_tangent_ws);
    float sin_theta_TL = clamp(dot(T, L), -1.0, 1.0);
    float sin_theta_TV = clamp(dot(T, V), -1.0, 1.0);

    out_color = vec4(vec3(0.98, 0.549, 0.2078), 1.0);
}