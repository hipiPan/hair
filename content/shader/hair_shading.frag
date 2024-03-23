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

// https://pbr-book.org/4ed/Reflection_Models/Scattering_from_Hair
// https://github.com/mmp/pbrt-v3
// [Marschner et al. 2003, "Light Scattering from Human Hair Fibers"]
void main()
{
    vec3 sun_direction = normalize(vec3(0.0, -1.0, -1.0));
    vec3 L = normalize(-sun_direction);
    vec3 V = normalize(view_buffer.view_position.xyz - in_position_ws);
    vec3 T = normalize(in_tangent_ws);
    float sin_theta_i = clamp(dot(T, L), -1.0, 1.0);
    float cos_theta_i = sqrt(1.0 - sin_theta_i * sin_theta_i);
    float sin_theta_o = clamp(dot(T, V), -1.0, 1.0);
    float cos_theta_o = sqrt(1.0 - sin_theta_o * sin_theta_o);
    float sin_theta_ip , cos_theta_ip, sin_a, cos_a;

    float shift = 0.035;
    float alpha[] =
    {
        -shift * 2,
        shift,
        shift * 4,
    };

    float roughness = 0.1;
    float beta[] =
    {
        (roughness * roughness),
        (roughness * roughness) / 2,
        (roughness * roughness) * 2,
    };

    float eta = 1.55;
    float etap = sqrt((eta * eta) - (sin_theta_o * sin_theta_o)) / cos_theta_o;
    float h = 0.0; // sin_gamma_o
    float gamma_o = asin(h);
    float sin_gamma_t = h / etap;
    float gamma_t = asin(sin_gamma_t);

    vec3 s = vec3(0);
    // R
    {
        sin_a = sin(alpha[0]);
        cos_a = cos(alpha[0]);
        sin_theta_ip = sin_theta_i * cos_a + cos_theta_i * sin_a;
        cos_theta_ip = cos_theta_i * cos_a - sin_theta_i * sin_a;
    }

    // TT
    {
        sin_a = sin(alpha[1]);
        cos_a = cos(alpha[1]);
        sin_theta_ip = sin_theta_i * cos_a + cos_theta_i * sin_a;
        cos_theta_ip = cos_theta_i * cos_a - sin_theta_i * sin_a;
    }

    // TRT
    {
        sin_a = sin(alpha[2]);
        cos_a = cos(alpha[2]);
        sin_theta_ip = sin_theta_i * cos_a + cos_theta_i * sin_a;
        cos_theta_ip = cos_theta_i * cos_a - sin_theta_i * sin_a;
    }

    out_color = vec4(vec3(0.98, 0.549, 0.2078), 1.0);
}