#ifndef HAIR_SHADER_DEFS_H
#define HAIR_SHADER_DEFS_H

#define normalize_safe(x) (x * inversesqrt(max(1e-7, dot(x, x))))

#endif