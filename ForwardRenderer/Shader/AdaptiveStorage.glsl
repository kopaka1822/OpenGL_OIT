
#ifdef SSBO_STORAGE
#include "uniforms/transform.glsl"
int getIndexFromVec(ivec3 c)
{
	return c.y * int(u_screenWidth) * int(MAX_SAMPLES) + c.x * int(MAX_SAMPLES) + c.z;
}
#endif

#ifdef STORAGE_READ_ONLY

// visibility function (xy = fragment xy, z = depth index)
#ifdef SSBO_STORAGE
#ifdef SSBO_TEX_VIEW
layout(binding = 7) uniform samplerBuffer tex_vis;
#define LOAD(coord) texelFetch(tex_vis, getIndexFromVec(coord)).xy
#else
layout(binding = 7, std430) restrict readonly buffer ssbo_fragmentBuffer
{
	vec2 buf_fragments[];
};
#define LOAD(coord) buf_fragments[getIndexFromVec(coord)]
#endif

#else

layout(binding = 7) uniform sampler3D tex_vis; // .x = depth, .y = transmittance
#define LOAD(coord) texelFetch(tex_vis, coord, 0).xy

#endif

#else // STOREAGE_READ_WRITE

// visibility function (xy = fragment xy, z = depth index)
#ifdef SSBO_STORAGE
layout(binding = 7, std430) coherent restrict buffer ssbo_fragmentBuffer
{
	vec2 buf_fragments[];
};

#define LOAD(coord) buf_fragments[getIndexFromVec(coord)]
#define STORE(coord, value) buf_fragments[getIndexFromVec(coord)] = value
#else
layout(binding = 0, rg32f) coherent uniform image3D tex_vis; // .x = depth, .y = transmittance
#define LOAD(coord) imageLoad(tex_vis, coord).xy
#define STORE(coord, value) imageStore(tex_vis, coord, vec4(value, 0.0, 0.0))
#endif

#endif
