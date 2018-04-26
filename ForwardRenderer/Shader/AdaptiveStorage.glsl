
#ifdef SSBO_STORAGE
#include "uniforms/transform.glsl"
int getIndexFromVec(int c)
{
	return int(gl_FragCoord.y) * int(u_screenWidth) * int(MAX_SAMPLES) + int(gl_FragCoord.x) * int(MAX_SAMPLES) + c;
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
#define LOAD(coord) texelFetch(tex_vis, ivec3(gl_FragCoord.xy, coord), 0).xy

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
#define LOAD(coord) imageLoad(tex_vis, ivec3(gl_FragCoord.xy, coord)).xy
#define STORE(coord, value) imageStore(tex_vis, ivec3(gl_FragCoord.xy, coord), vec4(value, 0.0, 0.0))
#endif

#endif

#ifdef USE_ARRAY_LINKED_LIST

struct Link
{
	float depth;
	float alpha;
	int next;
};

Link unpackLink(vec2 v)
{
	Link res;
	res.depth = v.x;
	vec2 alphaNext = unpackUnorm2x16( floatBitsToUint(v.y) );
	res.alpha = alphaNext.x;
	res.next = floatBitsToInt(alphaNext.y);
	return res;
}

// stuff depth into x
// stuff alpha into the first half of y
// stuff next into the second half of y
vec2 packLink(Link l)
{
	vec2 res;
	res.x = l.depth;
	float nextInDisguise = intBitsToFloat(l.next);
	res.y = uintBitsToFloat(packUnorm2x16(vec2(l.alpha, nextInDisguise)));
	return res;
}

#endif
