layout(location = 0) out vec4 out_fragColor;

// visibility function (xy = fragment xy, z = depth index)
#ifdef SSBO_STORAGE

#include "uniforms/transform.glsl"
int getIndexFromVec(ivec3 c)
{
	return c.y * int(u_screenWidth) * int(MAX_SAMPLES) + c.x * int(MAX_SAMPLES) + c.z;
}

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

void main()
{
	
	int maxZ = MAX_SAMPLES;
	float transmittance = 
		LOAD(ivec3(gl_FragCoord.xy, maxZ - 1)).y;

	out_fragColor = vec4(0.0, 0.0, 0.0, transmittance);
}