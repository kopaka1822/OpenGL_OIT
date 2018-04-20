layout(location = 0) out vec4 out_fragColor;

#include "uniforms/transform.glsl"

#ifndef UNSORTED_LIST
// visibility function (xy = fragment xy, z = depth index)
#ifdef SSBO_STORAGE
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

#else // unsorted list

// visibility function (xy = fragment xy, z = depth index)
#ifdef SSBO_STORAGE
layout(binding = 7, std430) coherent restrict buffer ssbo_fragmentBuffer
{
	vec2 buf_fragments[];
};

int getIndexFromVec(ivec3 c)
{
	return c.y * int(u_screenWidth) * int(MAX_SAMPLES) + c.x * int(MAX_SAMPLES) + c.z;
}

#define LOAD(coord) buf_fragments[getIndexFromVec(coord)]
#define STORE(coord, value) buf_fragments[getIndexFromVec(coord)] = value
#else
layout(binding = 0, rg32f) coherent uniform image3D tex_vis; // .x = depth, .y = transmittance
#define LOAD(coord) imageLoad(tex_vis, coord).xy
#define STORE(coord, value) imageStore(tex_vis, coord, vec4(value, 0.0, 0.0))
#endif

void main()
{
	// load and sort function
	vec2 fragments[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
		fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
		
		// sort values depending on depth
	// modified insertion sort
	for(int i = 1; i < MAX_SAMPLES; ++i)
	{
		// i - 1 elements are sorted
#pragma optionNV (unroll all)	
		for(int j = i; j > 0 && fragments[j - 1].x > fragments[j].x; --j)
		{
			vec2 tmp = fragments[j];
			fragments[j] = fragments[j - 1];
			fragments[j - 1] = tmp;
		}
#pragma optionNV (unroll)
	}
	
	float prevAlpha = 1.0;
	// accumulate alpha
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		prevAlpha *= fragments[i].y;
		fragments[i].y = prevAlpha;
	}
	
	// store values
	for(int i = 0; i < MAX_SAMPLES; ++i)
		STORE(ivec3(gl_FragCoord.xy, i), fragments[i]);
		
	out_fragColor = vec4(0.0, 0.0, 0.0, fragments[MAX_SAMPLES - 1].y);
}

#endif