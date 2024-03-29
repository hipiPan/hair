#version 450
#extension GL_ARB_separate_shader_objects : enable

#define sqr(x) pow(x, 2)
#define PI 3.1415926

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

// Modified Bessel function
float I0(float x)
{
    x = abs(x);
    float a;
    if (x < 3.75)
    {
        float t = x / 3.75;
        float t2 = t * t;
        a = 0.0045813;
        a = a * t2 + 0.0360768;
        a = a * t2 + 0.2659732;
        a = a * t2 + 1.2067492;
        a = a * t2 + 3.0899424;
        a = a * t2 + 3.5156229;
        a = a * t2 + 1.0;
    }
    else
    {
        float t = 3.75 / x;
        a = 0.00392377;
        a = a * t - 0.01647633;
        a = a * t + 0.02635537;
        a = a * t - 0.02057706;
        a = a * t + 0.00916281;
        a = a * t - 0.00157565;
        a = a * t + 0.00225319;
        a = a * t + 0.01328592;
        a = a * t + 0.39894228;
        a *= exp(x) * inversesqrt(x);
    }
    return a;
}

float LogI0(float x)
{
    if (x > 12.0)
        return x + 0.5 * (-log(2 * PI) + log(1 / x) + 1 / (8 * x));
    return log(I0(x));
}

float Mp(float v, float sin_theta_i, float cos_theta_i, float sin_theta_o, float cos_theta_o)
{
    float a = cos_theta_i * cos_theta_o / v;
    float b = sin_theta_i * sin_theta_o / v;
    float mp = 0.0;
    if (v <= 0.1)
        mp = (exp(LogI0(a) - b - 1 / v + 0.6931 + log(1 / (2 * v))));
    else
        mp =(exp(-b) * I0(a)) / (sinh(1 / v) * 2 * v);
    return mp;
}

float Phi(int p , float gamma_o , float gamma_t )
{
    return 2.0 * p * gamma_t - 2.0 * gamma_o + p * PI;
}

float Logistic(float x, float scale)
{
    x = abs(x);
    return exp(-x / scale) / (scale * sqr(1.0 + exp( -x / scale)));
}

float LogisticCDF(float x , float scale)
{
    return 1.0 / ( 1.0 + exp(-x / scale));
}

float TrimmedLogistic(float x, float scale, float a, float b)
{
    return Logistic(x, scale) / (LogisticCDF(b, scale) - LogisticCDF(a, scale));
}

float Np(int p, float phi, float scale, float gamma_o, float gamma_t)
{
    float dphi = phi - Phi(p, gamma_o, gamma_t);
    while (dphi > PI) dphi -= 2 * PI;
    while (dphi < -PI) dphi += 2 * PI;
    return TrimmedLogistic(dphi, scale, -PI, PI);
}

float SchlickFresnel(float eta, float cos_theta)
{
    float F0 = sqr(1 - eta) / sqr(1 + eta);
    return F0 + (1 - F0) * pow(1 - cos_theta, 5);
}

void Ap(float cos_theta_o, float cos_gamma_o, float eta, vec3 T, out vec3 ap0, out vec3 ap1, out vec3 ap2)
{
    float cos_theta = cos_theta_o * cos_gamma_o;
    float f = SchlickFresnel(eta, cos_theta);
    ap0 = vec3(f);
    ap1 = T * sqr(1.0 - f);
    ap2 = ap1 * T * f;
}

void main()
{
    vec3 sun_direction = normalize(vec3(0.0, 0.0, -1.0));
    vec3 L = normalize(-sun_direction);
    vec3 V = normalize(view_buffer.view_position.xyz - in_position_ws);
    vec3 T = normalize(in_tangent_ws);
    float sin_theta_i = clamp(dot(T, L), -1.0, 1.0);
    float cos_theta_i = sqrt(1.0 - sin_theta_i * sin_theta_i);
    float sin_theta_o = clamp(dot(T, V), -1.0, 1.0);
    float cos_theta_o = sqrt(1.0 - sin_theta_o * sin_theta_o);
    float sin_theta_ip , cos_theta_ip, sin_a, cos_a;

    vec3 Lp = L - sin_theta_i * T;
    vec3 Vp = V - sin_theta_o * T;
    float cos_phi = dot(Lp, Vp) * inversesqrt(dot(Lp, Lp) * dot(Vp, Vp));
    float phi = acos(cos_phi);

    float shift = 0.035;
    float alpha[] =
    {
        -shift * 2,
        shift,
        shift * 4,
    };

    float roughness = 0.1;
    vec3 v;
    v.x = sqr(0.726 * roughness + 0.812 * sqr(roughness) + 3.7 * pow(roughness, 20));
    v.y = 0.25 * v.x;
    v.z = 4.0 * v.x;
    float sqrt_pi_over8 = 0.626657069f; //sqrt( PI / 8.0f );
    float scale = sqrt_pi_over8 * (0.265 * roughness + 1.194 * sqr(roughness) + 5.372 * pow(roughness, 22));

    float eta = 1.55;
    float etap = sqrt((eta * eta) - (sin_theta_o * sin_theta_o)) / cos_theta_o;
    float h = 0.0; // sin_gamma_o
    float sin_theta_t = sin_theta_o / eta;
    float cos_theta_t = sqrt(1.0 - sin_theta_t * sin_theta_t);
    float gamma_o = asin(h);
    float cos_gamma_o = cos(gamma_o);
    float sin_gamma_t = h / etap;
    float cos_gamma_t = sqrt(1.0 - sin_gamma_t * sin_gamma_t);
    float gamma_t = asin(sin_gamma_t);

    vec3 sigma_a = vec3(0.06, 0.1, 0.2);
    vec3 expT = exp(sigma_a * (-2.0 * cos_gamma_t / cos_theta_t));
    vec3 ap0, ap1, ap2;
    Ap(cos_theta_o, cos_gamma_o, eta, expT, ap0, ap1, ap2);

    vec3 s = vec3(0);
    // R
    {
        sin_a = sin(alpha[0]);
        cos_a = cos(alpha[0]);
        sin_theta_ip = sin_theta_i * cos_a + cos_theta_i * sin_a;
        cos_theta_ip = cos_theta_i * cos_a - sin_theta_i * sin_a;
        cos_theta_ip = abs(cos_theta_ip);
        s += ap0 *
            Mp(v.x, sin_theta_ip, cos_theta_ip, sin_theta_o, cos_theta_o) *
            Np(0, phi, scale, gamma_o, gamma_t);
    }

    // TT
    {
        sin_a = sin(alpha[1]);
        cos_a = cos(alpha[1]);
        sin_theta_ip = sin_theta_i * cos_a + cos_theta_i * sin_a;
        cos_theta_ip = cos_theta_i * cos_a - sin_theta_i * sin_a;
        cos_theta_ip = abs(cos_theta_ip);
        s += ap1 *
            Mp(v.y, sin_theta_ip, cos_theta_ip, sin_theta_o, cos_theta_o) *
            Np(1, phi, scale, gamma_o, gamma_t);
    }

    // TRT
    {
        sin_a = sin(alpha[2]);
        cos_a = cos(alpha[2]);
        sin_theta_ip = sin_theta_i * cos_a + cos_theta_i * sin_a;
        cos_theta_ip = cos_theta_i * cos_a - sin_theta_i * sin_a;
        cos_theta_ip = abs(cos_theta_ip);
        s += ap2 *
            Mp(v.z, sin_theta_ip, cos_theta_ip, sin_theta_o, cos_theta_o) *
            Np(2, phi, scale, gamma_o, gamma_t);
    }

    out_color = vec4(s * 20, 1.0);
    //out_color = vec4(vec3(0.98, 0.549, 0.2078), 1.0);
}