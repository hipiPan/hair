#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;

layout(std140, set = 0, binding = 1) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    mat4 pod0;
    mat4 pod1;
} view_buffer;

void main() 
{
    gl_Position = view_buffer.proj_matrix * view_buffer.view_matrix * vec4(in_position, 1.0);
}