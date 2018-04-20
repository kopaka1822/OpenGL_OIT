layout(early_fragment_tests) in;

#include "MultiLayerAlphaSettings.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

// visibility function (x = depth, y = color)

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
layout(binding = 0, rg32f) coherent uniform image3D tex_fragments; // .x = depth, .y = color (rgba as uint)
#define LOAD(coord) imageLoad(tex_fragments, coord).xy
#define STORE(coord, value) imageStore(tex_fragments, coord, vec4(value, 0.0, 0.0))
#endif

layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

float packColor(vec4 color)
{
	return uintBitsToFloat(packUnorm4x8(color));
}

vec4 unpackColor(float f)
{
	return unpackUnorm4x8( floatBitsToUint(f) );
}

vec2 merge(vec2 front, vec2 back)
{
	float mergedDepth = front.x;
	vec4 colorFront = unpackColor(front.y);
	vec4 colorBack  = unpackColor(back.y);
	vec3 mergedRgb = colorFront.rgb + colorBack.rgb * colorBack.a;
	float mergedAlpha = colorFront.a * colorBack.a;
	
	return vec2(mergedDepth, packColor(vec4(mergedRgb, mergedAlpha)));
}

// color: color with transmittance instead op alpha (1 - alpha)
// color is also pre multiplied with alpha
void insertFragment(vec4 color, float depth)
{
	int size = MAX_SAMPLES;
	
#ifdef STORE_UNSORTED
	vec2 fragments[MAX_SAMPLES_C + 1];
	fragments[MAX_SAMPLES] = vec2(depth, packColor(color));
	
	// load function
	for(int i = 0; i < size; ++i){
		fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	}
	
	// x = depth, y = color
	vec2 high = vec2(-1.0, 0.0);
	int highIdx = 0;
	// second highest
	vec2 shigh = vec2(-1.0, 0.0);
	int shighIdx = 0;
	
	// find maximum
	for(int i = 0; i <= size; ++i)
	{
		if(fragments[i].x > high.x)
		{
			high = fragments[i];
			highIdx = i;
		}
	}
	
	// find second highest value
	for(int i = 0; i <= size; ++i)
	{
		if(fragments[i].x > shigh.x && i != highIdx)
		{
			shigh = fragments[i];
			shighIdx = i;
		}
	}
	
	// merge the two highest fragments
	vec2 merged = merge(shigh, high);

	// is the merged fragment in range?

	if(highIdx >= MAX_SAMPLES)
	{
		// the inserted value was merged immediately
		// just overwrite the second highest value
		highIdx = shighIdx;
		shighIdx = MAX_SAMPLES;
	}

	STORE(ivec3(gl_FragCoord.xy, highIdx), merged);
	if(shighIdx != MAX_SAMPLES)
	{
		STORE(ivec3(gl_FragCoord.xy, shighIdx), vec2(depth, packColor(color)));
	}
	

	/*vec2 insertValue = vec2(depth, packColor(color));
	if(highIdx >= MAX_SAMPLES)
	{
		// the inserted value was merged immediately
		// just overwrite the second highest value
		highIdx = shighIdx;
		// store the same value twice
		insertValue = merged;
	}

	STORE(ivec3(gl_FragCoord.xy, highIdx), merged);
	STORE(ivec3(gl_FragCoord.xy, shighIdx), insertValue);
*/
	// merge the two lowest
	/*if(highIdx < MAX_SAMPLES)
	{
		// high index is in range
		STORE(ivec3(gl_FragCoord.xy, highIdx), merged);
		// add the new item?
		if(shighIdx != MAX_SAMPLES) // highIdx != MAX_SAMPLES => new value must be stored
		{
			// this slot is now free
			STORE(ivec3(gl_FragCoord.xy, shighIdx), vec2(depth, packColor(color)));
		}
	}
	else // highIdx == MAX_SAMPLES
	{
		// only shighIdx is in range and the new inserted value was merged into the highest value
		STORE(ivec3(gl_FragCoord.xy, shighIdx), merged);
	}*/
	
#else // Store sorted
	vec2 fragments[MAX_SAMPLES_C + 1];
	
//#define INSERTION
#ifdef INSERTION
	// 1 pass insertion sort
	
	vec2 fragment = vec2(depth, packColor(color));
		
	// load function	
	// Version 1 (Standart)	
	//for(int i = 0; i < size; ++i)
	//	fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	//
	//int j = size;
	//for(; j > 0 && fragments[j - 1].x > fragment.x; --j)
	//{
	//	fragments[j] = fragments[j - 1];
	//}
	//fragments[j] = fragment;
	
	// Version 2 (No early out)
	for(int i = 0; i < size; ++i)
		fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	int j = size;
	int i = j;
	for(; j > 0; --j)
	{
		if(fragments[j - 1].x > fragment.x)
		{
			fragments[j] = fragments[j - 1];
			--i;
		}
	}
	
	//fragments[i] = fragment;
	
	// insert at i
	for(j = 0; j <= size; ++j)
	{
		if(i == j)
		{
			fragments[j] = fragment;
		}
	}
	
	//fragments[i] = fragment;
	
	// Version 3 better conditional move
	//for(int i = 0; i < size; ++i)
	//	fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	//int j = size;
	//int i = j;
	//for(; j > 0; --j)
	//{
	//	bool c = bool(fragments[j - 1].x > fragment.x);
	//	i -= int(c);
	//	if(c)
	//	{
	//		fragments[j] = fragments[j - 1];
	//	}
	//}
	//fragments[i] = fragment;
	//
	// Version 4 2-loop in positive direction
	//for(int i = 0; i < size; ++i)
	//	fragments[i + 1] = LOAD(ivec3(gl_FragCoord.xy, i));
	//
	//int i = 0;
	//for(int j = 0; j < size; ++j)
	//{
	//	if(fragments[j + 1].x <= fragment.x)
	//	{
	//		fragments[j] = fragments[j + 1];
	//		++i;
	//	}
	//}
	//
	////fragments[i] = fragment;
	//
	//for(int j = 0; j <= size; ++j)
	//{
	//	if(i == j)
	//	{
	//		fragments[j] = fragment;
	//	}
	//}
	
	//for(int j = 0; j < size;)
	
	//fragments[i] = fragment;
	//
	// Version 5 3-loop in positive direction
	//for(int i = 0; i < size; ++i)
	//	fragments[i + 1] = LOAD(ivec3(gl_FragCoord.xy, i));
	//
	//int j = 0;
	//int i = j;
	//for(; j < size; ++j)
	//{
	//	bool c = bool(fragments[j + 1].x <= fragment.x);
	//	i += int(c);
	//	if(c)
	//	{
	//		fragments[j] = fragments[j + 1];
	//	}
	//}
	//
	
	//fragments[i] = fragment;
	//for(j = 0; j <= size; ++j)
	//	if(i == j)
	//		fragments[j] = fragment;
	
	// Version 6 wie version 2 mit einem loop

	//for(int i = 0; i < size; ++i)
	//	fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	//	
	//fragments[MAX_SAMPLES] = fragment;
	//
	//int j = size;
	//for(; j > 0; --j)
	//{
	//	if(fragments[j - 1].x > fragment.x)
	//	{
	//		fragments[j] = fragments[j - 1];
	//		fragments[j - 1] = fragment;
	//	}
	//}
	

#else	
	fragments[0] = vec2(depth, packColor(color));
	
	// load function
	for(int i = 0; i < size; ++i){
		fragments[i + 1] = LOAD(ivec3(gl_FragCoord.xy, i));
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
		//else break;
	}
#endif
	
	fragments[size - 1] = merge(fragments[size - 1], fragments[size]);
	
	// write back function
	for(int i = 0; i < size; ++i)
		STORE(ivec3(gl_FragCoord.xy, i), fragments[i]);

#endif
}

void main()
{
	float dissolve = calcMaterialAlpha();
	vec3 color = calcMaterialColor();
	
	float dist = distance(u_cameraPosition, in_position);
	
	if(dissolve > 0.0 && !gl_HelperInvocation) // is it even visible?
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