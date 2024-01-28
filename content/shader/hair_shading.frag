#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(vec3(0.98, 0.549, 0.2078), 1.0);
}