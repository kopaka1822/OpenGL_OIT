layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

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
	float dissolve = calcMaterialAlpha();
	vec3 color = calcMaterialColor();
	
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