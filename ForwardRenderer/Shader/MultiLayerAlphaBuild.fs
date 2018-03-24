#version 450 core
//#pragma optionNV unroll all

#define MAX_SAMPLES 8

layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "uniforms/transform.glsl"
#include "uniforms/material.glsl"

layout(binding = 0) uniform sampler2D tex_ambient;
layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;
layout(binding = 3) uniform sampler2D tex_specular;

// visibility function (xy = fragment xy, z = depth index)
layout(binding = 0, rg32f) coherent uniform image3D tex_fragments; // .x = depth, .y = color (rgba as uint)
layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

float packColor(vec4 color)
{
	return uintBitsToFloat(packUnorm4x8(color));
}

vec4 unpackColor(float f)
{
	return unpackUnorm4x8( floatBitsToUint(f) );
}

// color: color with transmittance instead op alpha (1 - alpha)
// color is also pre multiplied with alpha
void insertFragment(vec4 color, float depth)
{
	vec2 fragments[MAX_SAMPLES + 1];
	fragments[0] = vec2(depth, packColor(color));
	
	int size = MAX_SAMPLES;
	
	// load function
	for(int i = 0; i < size; ++i){
		fragments[i + 1] = imageLoad(tex_fragments, ivec3(gl_FragCoord.xy, i)).xy;
	}
	
	// 1-pass bubble sort to insert fragment
	for(int i = 0; i < size; ++i)
	{
		if(fragments[i].x > fragments[i + 1].x)
		{
			// swap
			vec2 temp = fragments[i];
			fragments[i] = fragments[i + 1];
			fragments[i + 1] = temp;
		}
		// list is sorted
		else break;
	}
	
	// Compression (merge last two rows)
	float mergedDepth = fragments[size - 1].x;
	vec4 colorFront = unpackColor(fragments[size - 1].y);
	vec4 colorBack  = unpackColor(fragments[size].y);
	vec3 mergedRgb = colorFront.rgb + colorBack.rgb * colorBack.a;
	float mergedAlpha = colorFront.a * colorBack.a;
	
	fragments[size - 1] = vec2(mergedDepth, packColor(vec4(mergedRgb, mergedAlpha)));
	
	// write back function
	for(int i = 0; i < size; ++i)
		imageStore(tex_fragments, ivec3(gl_FragCoord.xy, i), vec4(fragments[i], 0.0, 0.0));
}

void main()
{
	vec3 LIGHT_DIR = normalize(in_position - u_cameraPosition);

	vec3 ambient_col = m_ambient.rgb * texture(tex_ambient, in_texcoord).rgb;
	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	vec3 diffuse_col = m_diffuse.rgb * texture(tex_diffuse, in_texcoord).rgb;
	vec3 specular_col = m_specular.rgb * texture(tex_specular, in_texcoord).rgb;
	
	// take the diffuse texture alpha since its sometimes meant to be the alpha
	dissolve *= texture(tex_diffuse, in_texcoord).a;
	
	vec3 normal = normalize(in_normal);
	
	// angle for diffuse
	float cosTheta = dot(-LIGHT_DIR, normal);
	// angle for specular color
	vec3 viewDir = normalize(in_position - u_cameraPosition);
	float HdotN = dot(normalize(viewDir + LIGHT_DIR), normal);
	
	vec3 color = 
		max(vec3(0.0), ambient_col) + // ambient color
		max(vec3(0.0), diffuse_col * cosTheta) + // diffuse color
		max(vec3(0.0), specular_col * pow(max(0.0, -HdotN), m_specular.a)) +
		diffuse_col * 0.01;
	
	float dist = distance(u_cameraPosition, in_position);
	
	if(dissolve > 0.0 && !gl_HelperInvocation) // is it event visible?
	{
		
		bool keepWaiting = true;
		while(keepWaiting)
		{
			// acquire lock
			if(imageAtomicCompSwap(tex_atomics, ivec2(gl_FragCoord.xy), 0u, 1u) == 0)
			{
				insertFragment(vec4(dissolve * color, 1.0 - dissolve), dist);
				
				memoryBarrier();
				imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 0u);
				keepWaiting = false;
			}
		}
	}
	
	out_fragColor = vec4(0.0);
}