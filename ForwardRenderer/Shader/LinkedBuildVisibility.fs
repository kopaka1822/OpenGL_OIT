#version 440 core

layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "uniforms/transform.glsl"
#include "uniforms/material.glsl"

// only sampler important for dissolve
layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;

layout(binding = 4) uniform atomic_uint atomic_counter;
layout(binding = 0, r32ui) coherent uniform uimage2D tex_atomics;

struct BufferData
{
	float invAlpha;
	float depth;
	uint next;
};

layout(binding = 3, std430) writeonly buffer buf_visz
{
	BufferData visz_data[];
};

void main()
{
	
	
	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	
	// take the diffuse texture alpha since its sometimes meant to be the alpha
	dissolve *= texture(tex_diffuse, in_texcoord).a;
	float dist = distance(u_cameraPosition, in_position);
	if(dissolve >= 0.0) // is it even visible?
	{
		uint index = atomicCounterIncrement(atomic_counter) + 1u;
		visz_data[index-1].invAlpha = 1.0 - dissolve;
		visz_data[index-1].depth = dist;
		visz_data[index-1].next = imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), index);
	}
	
	out_fragColor = vec4(0.0);
}