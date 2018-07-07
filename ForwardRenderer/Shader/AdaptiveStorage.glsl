
#define SSBO_GROUP_X 2
#define SSBO_GROUP_Y 4

#ifdef SSBO_STORAGE
#include "uniforms/transform.glsl"

#ifndef SSBO_GROUP_X
// use normal indexing
const int ssbo_offset = int(gl_FragCoord.y) * int(u_screenWidth) * int(MAX_SAMPLES) + int(gl_FragCoord.x) * int(MAX_SAMPLES);
int getIndexFromVec(int c)
{
	return ssbo_offset + c;
}
#else // use iterleaved ssbo

// screen is aligned by 4 bytes
const uint alignedWidth = (u_screenWidth + 3u) & ~(3u);

// NOTE: resize ssbo in adaptive.cpp by the offset multiplier as well
const uint offset_multiplier = 1;

// determine work group
const uvec2 ssbo_wg = uvec2(gl_FragCoord.xy) / uvec2(SSBO_GROUP_X, SSBO_GROUP_Y);
const uint ssbo_wg_id = ssbo_wg.y * (alignedWidth / uint(SSBO_GROUP_X)) + ssbo_wg.x;
const uint ssbo_wg_offset = ssbo_wg_id * MAX_SAMPLES * offset_multiplier * SSBO_GROUP_X * SSBO_GROUP_Y;

const uvec2 ssbo_local = uvec2(gl_FragCoord.xy) % uvec2(SSBO_GROUP_X, SSBO_GROUP_Y);
const uint ssbo_local_id = ssbo_local.y * SSBO_GROUP_X + ssbo_local.x;
const uint ssbo_stride = SSBO_GROUP_X * SSBO_GROUP_Y;

#ifdef USE_DEFAULT
// fully interleaved
uint getIndexFromVec(int c)
{
	return ssbo_wg_offset + ssbo_local_id + uint(c) * ssbo_stride;
}
#else // aligned interleaved
uint getIndexFromVec(int c)
{
	c *= int(offset_multiplier);
	// alignment elements are packed together to avoid lost update
	const uint alignment = 4u;
	uint mc = uint(c) % alignment;
	uint dc = uint(c) / alignment;
	return ssbo_wg_offset + ssbo_local_id * alignment + dc * ssbo_stride * alignment + mc;
}
#endif // use default
#endif // ssbo goup x
#endif // ssbo storage

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

	// int important for arithmetical shift
	int pcked = floatBitsToInt(v.y);
	res.next = pcked >> 16;

	vec2 alphaNext = unpackUnorm2x16(pcked);
	res.alpha = alphaNext.x;
	return res;
}

// stuff depth into x
// stuff alpha into the first half of y
// stuff next into the second half of y
vec2 packLink(Link l)
{
	vec2 res;
	res.x = l.depth;
	uint pcked = packUnorm2x16(vec2(l.alpha, 0.0f));
	pcked |= l.next << 16;
	res.y = uintBitsToFloat(pcked);
	return res;
}

#endif
