#version 450

#define MAX_SAMPLES 32

layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

layout(binding = 0) uniform ubo_transform
{
	mat4 u_model;
	mat4 u_viewProjection;
	vec3 u_cameraPosition;
};

layout(binding = 1) uniform ubo_material
{
	vec3 m_ambient;
	float m_dissolve;
	vec4 m_diffuse;
	vec4 m_specular;
};

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
		if(gl_HelperInvocation) return;
		
		uint index = atomicCounterIncrement(atomic_counter) + 1u;
		visz_data[index-1].invAlpha = 1.0 - dissolve;
		visz_data[index-1].depth = dist;
		uint next = 0;
		uint prev = 1;
		
		
		do
		{
			// get current pointer
			next = imageAtomicAdd(tex_atomics, ivec2(gl_FragCoord.xy), 0);
			visz_data[index-1].next = next;
			prev = imageAtomicCompSwap(tex_atomics, ivec2(gl_FragCoord.xy), next, index);
			
		} while(next != prev);
	}
	
	out_fragColor = vec4(0.0);
}