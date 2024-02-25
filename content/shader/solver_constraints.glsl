#ifndef SOLVER_CONSTRAINTS_H
#define SOLVER_CONSTRAINTS_H

#ifndef W_EPSILON
#define W_EPSILON 1e-7
#endif

#ifndef inversesqrt_safe
#define inversesqrt_safe(x) max(0.0, inversesqrt(x))
#endif

void solve_distance_constraint(
    float distance0, float stiffness,
    float w0, float w1,
    vec3 p0, vec3 p1,
    inout vec3 d0, inout vec3 d1)
{
    vec3 r = p1 - p0;
    float rd_inv = inversesqrt_safe(dot(r, r));

    float delta = 1.0 - (distance0 * rd_inv);
    float w_inv = (delta * stiffness) / (w0 + w1 + W_EPSILON);

    d0 += (w0 * w_inv) * r;
    d1 -= (w1 * w_inv) * r;
}

void solve_distance_lra_constraint(
    float distance_max,
    vec3 p0, vec3 p1,
    inout vec3 d1)
{
    // Long Range Attachments
    // https://matthias-research.github.io/pages/publications/sca2012cloth.pdf
    vec3 r = p1 - p0;
    float rd_inv = inversesqrt_safe(dot(r, r));

    r *= 1.0 - min(1.0, distance_max * rd_inv);

    d1 -= r;
}

#endif