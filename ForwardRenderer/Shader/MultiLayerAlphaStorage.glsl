#include "MultiLayerAlphaSettings.glsl"
// visibility function (x = depth, y = color)
#include "uniforms/transform.glsl"

#ifdef SSBO_STORAGE
int getIndexFromVec(int c)
{
	return int(gl_FragCoord.y) * int(u_screenWidth) * int(MAX_SAMPLES) + int(gl_FragCoord.x) * int(MAX_SAMPLES) + c;
}
#endif


#ifdef STORAGE_READ_ONLY

#ifdef SSBO_STORAGE
layout(binding = 7, std430) readonly buffer ssbo_fragmentBuffer
{
	vec2 buf_fragments[];
};
#define LOAD(coord) buf_fragments[getIndexFromVec(coord)]
#else
layout(binding = 7) uniform sampler3D tex_fragments; // .x = depth, .y = color (rgba as uint)
#define LOAD(coord) texelFetch(tex_fragments, coord, 0).xy
#endif

vec4 unpackColor(float f)
{
	return unpackUnorm4x8(floatBitsToUint(f));
}

#else // STORAGE_READ_WRITE

#ifdef SSBO_STORAGE
layout(binding = 7, std430) volatile buffer ssbo_fragmentBuffer
{
	vec2 buf_fragments[];
};
#define LOAD(coord) buf_fragments[getIndexFromVec(coord)]
#define STORE(coord, value) buf_fragments[getIndexFromVec(coord)] = value
#else
layout(binding = 0, rg32f) coherent uniform image3D tex_fragments; // .x = depth, .y = color (rgba as uint)
#define LOAD(coord) imageLoad(tex_fragments, coord).xy
#define STORE(coord, value) imageStore(tex_fragments, coord, vec4(value, 0.0, 0.0))
#endif

#endif