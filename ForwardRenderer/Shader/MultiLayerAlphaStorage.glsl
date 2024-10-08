#include "MultiLayerAlphaSettings.glsl"
// visibility function (x = depth, y = color)
#include "uniforms/transform.glsl"

#ifdef STORE_UNSORTED
#define SSBO_GROUP_X 2
#define SSBO_GROUP_Y 4
#else
#define SSBO_GROUP_X 2
#define SSBO_GROUP_Y 4
#endif

#ifdef SSBO_GROUP_X
// helper function for image atomic lock
ivec2 getLockIndex()
{
	return ivec2(
			int(gl_FragCoord.x) / SSBO_GROUP_X,
			int(gl_FragCoord.y) / SSBO_GROUP_Y
		);
}
#endif

#ifdef SSBO_STORAGE

#ifdef SSBO_GROUP_X

// screen is aligned by 4 bytes
const uint alignedWidth = (u_screenWidth + 3u) & ~(3u);

// determine work group
const uvec2 ssbo_wg = uvec2(gl_FragCoord.xy) / uvec2(SSBO_GROUP_X, SSBO_GROUP_Y);
const uint ssbo_wg_id = ssbo_wg.y * (alignedWidth / uint(SSBO_GROUP_X)) + ssbo_wg.x;
const uint ssbo_wg_offset = ssbo_wg_id * MAX_SAMPLES * SSBO_GROUP_X * SSBO_GROUP_Y;

const uvec2 ssbo_local = uvec2(gl_FragCoord.xy) % uvec2(SSBO_GROUP_X, SSBO_GROUP_Y);
const uint ssbo_local_id = ssbo_local.y * SSBO_GROUP_X + ssbo_local.x;
const uint ssbo_stride = SSBO_GROUP_X * SSBO_GROUP_Y;

#ifdef STORE_UNSORTED
uint getIndexFromVec(int c)
{
	// alignment elements are packed together to avoid lost update
	const uint alignment = 4u;
	uint mc = uint(c) % alignment;
	uint dc = uint(c) / alignment;
	return ssbo_wg_offset + ssbo_local_id * alignment + dc * ssbo_stride * alignment + mc;
}
#else
uint getIndexFromVec(int c)
{
	return ssbo_wg_offset + ssbo_local_id + uint(c) * ssbo_stride;
}
#endif
#else
const int ssbo_offset = int(gl_FragCoord.y) * int(u_screenWidth) * int(MAX_SAMPLES) + int(gl_FragCoord.x) * int(MAX_SAMPLES);
int getIndexFromVec(int c)
{
	return ssbo_offset + c;
}
#endif
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
#define LOAD(coord) texelFetch(tex_fragments, ivec3(gl_FragCoord.xy, coord), 0).xy
#endif

vec4 unpackColor(float f)
{
	return unpackUnorm4x8(floatBitsToUint(f));
}

#else // STORAGE_READ_WRITE

#ifdef SSBO_STORAGE
layout(binding = 7, std430) coherent buffer ssbo_fragmentBuffer
{
	vec2 buf_fragments[];
};
#define LOAD(coord) buf_fragments[getIndexFromVec(coord)]
#define STORE(coord, value) buf_fragments[getIndexFromVec(coord)] = value
#else
layout(binding = 0, rg32f) coherent uniform image3D tex_fragments; // .x = depth, .y = color (rgba as uint)
#define LOAD(coord) imageLoad(tex_fragments, ivec3(gl_FragCoord.xy, coord)).xy
#define STORE(coord, value) imageStore(tex_fragments, ivec3(gl_FragCoord.xy, coord), vec4(value, 0.0, 0.0))
#endif

#endif