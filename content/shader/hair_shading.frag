#version 450
#extension GL_ARB_separate_shader_objects : enable

#define sqr(x) pow(x, 2)
#define PI 3.1415926

layout(location = 0) in highp vec3 in_position_ws;
layout(location = 1) in highp vec3 in_bitangent_ws;
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

float DieletricFresnel(float n1, float n2, float cos_r, float cos_t)
{
    float parallel = (n2 * cos_r - n1 * cos_t) / (n2 * cos_r + n1 * cos_t);
    float perpendicular = (n1 * cos_r - n2 * cos_t) / (n1 * cos_r + n2 * cos_t);
    return (sqr(parallel) + sqr(perpendicular)) * 0.5;
}

void Ap(float cos_theta_o, float cos_gamma_o, float eta, vec3 T, out vec3 ap0, out vec3 ap1, out vec3 ap2)
{
    float cos_theta = cos_theta_o * cos_gamma_o;
    float sin_threa = sqrt(1.0 - cos_theta * cos_theta);
    float sin_threa_t = sin_threa / eta;
    float cos_threa_t = sqrt(1.0 - sin_threa_t * sin_threa_t);

    //float f = SchlickFresnel(eta, cos_theta);
    float f = DieletricFresnel(1.0, eta, cos_theta, cos_threa_t);
    ap0 = vec3(f);
    ap1 = T * sqr(1.0 - f);
    ap2 = ap1 * T * f;
}

struct HairContext
{
    float sin_theta_i;
    float cos_theta_i;
    float sin_theta_o;
    float cos_theta_o;
    float cos_phi;
    float phi;
    vec3 alpha;
    vec3 v;
    float scale;
    float eta;
    float etap;
    vec3 sigma_a;
};

vec3 hair_bsdf(const HairContext ctx, float h)
{
    // h = sin_gamma_o
    float sin_theta_t = ctx.sin_theta_o / ctx.eta;
    float cos_theta_t = sqrt(1.0 - sin_theta_t * sin_theta_t);
    float gamma_o = asin(h);
    float cos_gamma_o = cos(gamma_o);
    float sin_gamma_t = h / ctx.etap;
    float cos_gamma_t = sqrt(1.0 - sin_gamma_t * sin_gamma_t);
    float gamma_t = asin(sin_gamma_t);

    vec3 expT = exp(ctx.sigma_a * (-2.0 * cos_gamma_t / cos_theta_t));
    vec3 ap0, ap1, ap2;
    Ap(ctx.cos_theta_o, cos_gamma_o, ctx.eta, expT, ap0, ap1, ap2);

    vec3 s = vec3(0);
    float sin_theta_ip , cos_theta_ip, sin_a, cos_a;
    // R
    {
        sin_a = sin(ctx.alpha.x);
        cos_a = cos(ctx.alpha.x);
        sin_theta_ip = ctx.sin_theta_i * cos_a + ctx.cos_theta_i * sin_a;
        cos_theta_ip = ctx.cos_theta_i * cos_a - ctx.sin_theta_i * sin_a;
        cos_theta_ip = abs(cos_theta_ip);
        s += ap0 *
        Mp(ctx.v.x, sin_theta_ip, cos_theta_ip, ctx.sin_theta_o, ctx.cos_theta_o) *
        Np(0, ctx.phi, ctx.scale, gamma_o, gamma_t);
    }

    // TT
    {
        sin_a = sin(ctx.alpha.y);
        cos_a = cos(ctx.alpha.y);
        sin_theta_ip = ctx.sin_theta_i * cos_a + ctx.cos_theta_i * sin_a;
        cos_theta_ip = ctx.cos_theta_i * cos_a - ctx.sin_theta_i * sin_a;
        cos_theta_ip = abs(cos_theta_ip);
        s += ap1 *
        Mp(ctx.v.y, sin_theta_ip, cos_theta_ip, ctx.sin_theta_o, ctx.cos_theta_o) *
        Np(1, ctx.phi, ctx.scale, gamma_o, gamma_t);
    }

    // TRT
    {
        sin_a = sin(ctx.alpha.z);
        cos_a = cos(ctx.alpha.z);
        sin_theta_ip = ctx.sin_theta_i * cos_a + ctx.cos_theta_i * sin_a;
        cos_theta_ip = ctx.cos_theta_i * cos_a - ctx.sin_theta_i * sin_a;
        cos_theta_ip = abs(cos_theta_ip);
        s += ap2 *
        Mp(ctx.v.z, sin_theta_ip, cos_theta_ip, ctx.sin_theta_o, ctx.cos_theta_o) *
        Np(2, ctx.phi, ctx.scale, gamma_o, gamma_t);
    }

    return s;
}

void main()
{
    vec3 sun_direction = normalize(vec3(0.0, 0.5, -1.0));
    vec3 L = normalize(-sun_direction);
    vec3 V = normalize(view_buffer.view_position.xyz - in_position_ws);
    vec3 T = normalize(in_bitangent_ws);
    vec3 N = normalize(in_normal_ws);
    float sin_theta_i = clamp(dot(T, L), -1.0, 1.0);
    float cos_theta_i = sqrt(1.0 - sin_theta_i * sin_theta_i);
    float sin_theta_o = clamp(dot(T, V), -1.0, 1.0);
    float cos_theta_o = sqrt(1.0 - sin_theta_o * sin_theta_o);

    vec3 Lp = L - sin_theta_i * T;
    vec3 Vp = V - sin_theta_o * T;
    float cos_phi = dot(Lp, Vp) * inversesqrt(dot(Lp, Lp) * dot(Vp, Vp));
    float phi = acos(cos_phi);

    float shift = 0.035;
    vec3 alpha;
    alpha.x = -shift * 2;
    alpha.y = shift;
    alpha.z = shift * 4;

    float roughness = 0.2;
    vec3 v;
    v.x = sqr(0.726 * roughness + 0.812 * sqr(roughness) + 3.7 * pow(roughness, 20));
    v.y = 0.25 * v.x;
    v.z = 4.0 * v.x;
    float sqrt_pi_over8 = 0.626657069f; //sqrt( PI / 8.0f );
    float scale = sqrt_pi_over8 * (0.265 * roughness + 1.194 * sqr(roughness) + 5.372 * pow(roughness, 22));

    float eta = 1.55;
    float etap = sqrt((eta * eta) - (sin_theta_o * sin_theta_o)) / cos_theta_o;

    vec3 hair_color = vec3(0.7, 0.3, 0.1);
    vec3 sigma_a = vec3(0.05, 0.1, 0.2);

    HairContext ctx;
    ctx.sin_theta_i = sin_theta_i;
    ctx.cos_theta_i = cos_theta_i;
    ctx.sin_theta_o = sin_theta_o;
    ctx.cos_theta_o = cos_theta_o;
    ctx.cos_phi = cos_phi;
    ctx.phi = phi;
    ctx.alpha = alpha;
    ctx.v = v;
    ctx.scale = scale;
    ctx.eta = eta;
    ctx.etap = etap;
    ctx.sigma_a = sigma_a;

    float h = 0.0;
    uint h_count = 8;
    vec3 s = vec3(0);
    for (uint i = 0; i < h_count; i++)
    {
        h = ((i + 0.5) / float(h_count) * 2.0 - 1.0); // [-1, 1]
        s += hair_bsdf(ctx, h);
    }
    s = s / float(h_count);

    vec3 albedo = hair_color;
    vec3 d = albedo / PI;

    // Light params
    vec4 light_color_intensity = vec4(0.85, 0.85, 0.9, 130000.0); // Color and lux

    // Camera params
    float exposure = 0.0000260416691;

    light_color_intensity.w *= exposure;
    vec3 ambient = vec3(0.05);
    vec3 color = (d + s) * light_color_intensity.xyz * light_color_intensity.w * clamp(dot(N, L), -1.0, 1.0) + ambient;
    out_color = vec4(color, 1.0);
}