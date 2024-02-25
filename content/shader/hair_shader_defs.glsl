#ifndef HAIR_SHADER_DEFS_H
#define HAIR_SHADER_DEFS_H

#define normalize_safe(x) (x * inversesqrt(max(1e-7, dot(x, x))))

layout(push_constant) uniform ConstantBlock
{
    uint strand_count;
    uint strand_particle_count;
    uint strand_particle_stride;
    float particle_diameter;
} constant;

#endif