layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#define LIGHT_ONLY_TRANSPARENT
#include "light/light.glsl"

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

layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

float getRectArea(vec2 pos1, vec2 pos2)
{
	return (pos2.x - pos1.x) * (pos1.y - pos2.y);
}

#ifdef UNSORTED_LIST

struct Fragment
{
	float depth;
	float alpha;
	int oldPosition;
};

void insertAlpha(float one_minus_alpha, float depth)
{
	Fragment fragments[MAX_SAMPLES + 1];
	fragments[0] = Fragment(depth, one_minus_alpha, -1);
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		vec2 val = LOAD(ivec3(gl_FragCoord.xy, i));
		fragments[i + 1] = Fragment(val.x, val.y, i);
	}
	
	// sort values depending on depth
	// modified insertion sort
	for(int i = 1; i <= MAX_SAMPLES; ++i)
	{
		// i - 1 elements are sorted
#pragma optionNV (unroll all)	
		for(int j = i; j > 0 && fragments[j - 1].depth > fragments[j].depth; --j)
		{
			Fragment tmp = fragments[j];
			fragments[j] = fragments[j - 1];
			fragments[j - 1] = tmp;
		}
#pragma optionNV (unroll)
	}
	
	// find smallest rectangle
	// find smallest rectangle to insert
	float minRectArea = 1.0 / 0.0;
	Fragment smallestRectValue = fragments[0];
	Fragment nextSmallestRectValue = fragments[0];
	
	float prevAlpha = 1.0;
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		prevAlpha *= fragments[i].alpha;
		float nextAlpha = fragments[i + 1].alpha;
		float area = getRectArea( vec2(fragments[i].depth, prevAlpha),
								  vec2(fragments[i + 1].depth, prevAlpha * nextAlpha));
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectValue = fragments[i];
			nextSmallestRectValue = fragments[i + 1];
		}
	}
	
	// adjust smallestRectValue
	smallestRectValue.alpha = smallestRectValue.alpha * nextSmallestRectValue.alpha;
	bool storeSecond = true;

	if(nextSmallestRectValue.oldPosition == -1)
	{
		// the inserted value should not be stored
		storeSecond = false;
		// store same value twice
		nextSmallestRectValue = smallestRectValue;
	}
	else if(smallestRectValue.oldPosition == -1)
	{
		// overwrite nextSmallestRectValue.oldPosition
		smallestRectValue.oldPosition = nextSmallestRectValue.oldPosition;
		storeSecond = false;
		nextSmallestRectValue = smallestRectValue;
	} 
	else
	{
		// the inserted value should overwrite the nextSmallestRect
		nextSmallestRectValue.depth = depth;
		nextSmallestRectValue.alpha = one_minus_alpha;
	}
	
	STORE(ivec3(gl_FragCoord.xy, smallestRectValue.oldPosition),
		vec2(smallestRectValue.depth, smallestRectValue.alpha));
	
	//if(storeSecond)
		STORE(ivec3(gl_FragCoord.xy, nextSmallestRectValue.oldPosition),
			vec2(nextSmallestRectValue.depth, nextSmallestRectValue.alpha));
}

#else
void insertAlphaReference(float one_minus_alpha, float depth)
{
	vec2 fragments[MAX_SAMPLES + 1];
	
	// load values Upack AOIT Data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	}
	
	int insertPosition = 0;
	float prevAlpha = 1.0;
	
	// find insert index
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(fragments[i].x < depth)
		{
			insertPosition = i + 1;
			prevAlpha = fragments[i].y;
		}

	}
	
	// Make space for the new fragment
	for(int i = MAX_SAMPLES - 1; i >= 0; --i)
	{
		if(insertPosition <= i)
		{
			fragments[i + 1].x = fragments[i].x;
			fragments[i + 1].y = fragments[i].y * one_minus_alpha;
		}
	}
	
	// insert new fragment
	for(int i = 0; i <= MAX_SAMPLES; ++i)
	{
		if(insertPosition == i)
		{
			fragments[i].x = depth;
			fragments[i].y = one_minus_alpha * prevAlpha;
		}
	}
	
	float nodeUnderError[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		nodeUnderError[i] = getRectArea(fragments[i], fragments[i+1]);
	}
	
	// find the node that generates the smallest removal error
	int smallestErrorIndex = 0;
	float smallestError = nodeUnderError[0];
	
	for(int i = 1; i < MAX_SAMPLES; ++i)
	{
		if(nodeUnderError[i] < smallestError)
		{
			smallestError = nodeUnderError[i];
			smallestErrorIndex = i;
		}
	}
	
	// Remove that node
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestErrorIndex < i)
		{
			// adjust depth
			fragments[i].x = fragments[i + 1].x;
		}
	}
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestErrorIndex <= i)
		{
			// adjust alpha
			fragments[i].y = fragments[i + 1].y;
		}
	}
	
	// pack aoit data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		STORE(ivec3(gl_FragCoord.xy, i), fragments[i]);
	}
}

void insertAlpha(float one_minus_alpha, float depth)
{	
	//insertAlphaReference(one_minus_alpha, depth);
	//insertAlphaSep(one_minus_alpha, depth);
	//return;
	
	vec2 fragments[MAX_SAMPLES + 1];
	// load values
	fragments[0] = vec2(depth, one_minus_alpha);
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i + 1] = LOAD(ivec3(gl_FragCoord.xy, i));
	}
	
	// 1 pass bubble sort for new value
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		float newAlpha = fragments[i + 1].y * one_minus_alpha;
		if(fragments[i].x > fragments[i + 1].x)
		{
			// shift lower value and insert new value at i + 1
			fragments[i] = fragments[i + 1];
			fragments[i + 1].x = depth;
			fragments[i + 1].y = newAlpha;
		}
		else
		{
			// adjust values after the insert position
			fragments[i + 1].y = newAlpha;
		}
	}
	
	// find smallest rectangle
	// find smallest rectangle to insert
	int smallestRectPos = 0;
	float minRectArea = 1.0 / 0.0;
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		float area = getRectArea(	fragments[i],
									fragments[i+1] );
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectPos = i;
		}
	}
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestRectPos <= i)
		{
			fragments[i].y = fragments[i + 1].y;
		}
		if(smallestRectPos < i)
		{
			fragments[i] = fragments[i + 1];
		}
	}
	
	// pack aoit data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		STORE(ivec3(gl_FragCoord.xy, i), fragments[i]);
	}
}
#endif


void main()
{
	float dissolve = calcMaterialAlpha();
	
	float dist = distance(u_cameraPosition, in_position);
	if(dissolve > 0.0 && !gl_HelperInvocation) // is it event visible?
	{
		
		bool keepWaiting = true;
		while(keepWaiting)
		{
			// acquire lock
			if(imageAtomicCompSwap(tex_atomics, ivec2(gl_FragCoord.xy), 0u, 1u) == 0)
			{
				
				insertAlpha(1.0 - dissolve, dist); 
				
#ifdef SSBO_STORAGE
				memoryBarrierBuffer();
#else				
				memoryBarrierImage();
#endif
				imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 0u);
				keepWaiting = false;
			}
		}
	}
	out_fragColor = vec4(0.0);
}