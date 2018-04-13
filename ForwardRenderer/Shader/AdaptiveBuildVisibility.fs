layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#define LIGHT_ONLY_TRANSPARENT
#include "light/light.glsl"

// visibility function (xy = fragment xy, z = depth index)
layout(binding = 0, rg32f) volatile uniform image3D tex_visz; // .x = depth, .y = transmittance
layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

float getRectArea(vec2 pos1, vec2 pos2)
{
	return (pos2.x - pos1.x) * (pos1.y - pos2.y);
}

void insertAlphaReference(float one_minus_alpha, float depth)
{
	vec2 fragments[MAX_SAMPLES + 1];
	
	// load values Upack AOIT Data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i] = imageLoad(tex_visz, ivec3(gl_FragCoord.xy, i)).xy;
	}
	
	// 1 pass bubble for new value
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
	//fragments[insertPosition].x = depth;
	//fragments[insertPosition].y = one_minus_alpha * prevAlpha;
	
	float nodeUnderError[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		nodeUnderError[i] = abs(getRectArea(fragments[i], fragments[i+1]));
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
		imageStore(tex_visz, ivec3(gl_FragCoord.xy, i), vec4(fragments[i], 0.0, 0.0));
	}
}

void insertAlpha(float one_minus_alpha, float depth)
{	
	insertAlphaReference(one_minus_alpha, depth);
	return;

	vec2 fragments[MAX_SAMPLES + 1];
	// load values
	fragments[0] = vec2(depth, one_minus_alpha);
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i + 1] = imageLoad(tex_visz, ivec3(gl_FragCoord.xy, i)).xy;
	}
	
		// 1 pass bubble for new value
	int insertPosition = 0;
		
	while(insertPosition < MAX_SAMPLES && fragments[insertPosition].x > fragments[insertPosition + 1].x)
	{
		vec2 tmp = fragments[insertPosition];
		fragments[insertPosition] = fragments[insertPosition + 1];
		fragments[insertPosition + 1] = tmp;
		fragments[insertPosition + 1].y = fragments[insertPosition].y * one_minus_alpha;
		++insertPosition;
	}
	
	for(int i = insertPosition + 1; i <= MAX_SAMPLES; ++i)
	{
		fragments[i].y *= one_minus_alpha;
	}
	
	// find smallest rectangle
	// find smallest rectangle to insert
	int smallestRectPos = 0;
	float minRectArea = getRectArea( fragments[0], fragments[1] );
	
	for(int i = 1; i < MAX_SAMPLES; ++i)
	{
		float area = getRectArea(	fragments[i],
									fragments[i+1] );
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectPos = i;
		}
	}
	
	// do underestimation (samllestRectPos will get smaller)
	// everything before smallestRectPos will remain the same
	// store the new function
	

	// write the changed values
	/*for(int i = insertPosition; i < smallestRectPos; ++i)
	{
		imageStore(tex_visz, ivec3(gl_FragCoord.xy, i), vec4(fragments[i], 0.0, 0.0));
	}
	
	// do the underestimation
	// depth value from first, transmittance from second
	imageStore(tex_visz, ivec3(gl_FragCoord.xy, smallestRectPos), 
			vec4(fragments[smallestRectPos].x, fragments[smallestRectPos + 1].y, 0.0, 0.0));
	
	// everything after smallestRectPos will be shifted by one to the left
	for(int i = smallestRectPos + 1; i < MAX_SAMPLES; ++i)
	{
		imageStore(tex_visz, ivec3(gl_FragCoord.xy, i), vec4(fragments[i + 1], 0.0, 0.0));
	}*/
	
	fragments[smallestRectPos].y = fragments[smallestRectPos + 1].y;
	for(int i = min(insertPosition, smallestRectPos); i < MAX_SAMPLES; ++i)
	{
		imageStore(tex_visz, ivec3(gl_FragCoord.xy, i), vec4(fragments[i + int(i > smallestRectPos)], 0.0, 0.0));
	}
	
	//for(int i = smallestRectPos + 1; i < 0;)
}

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
				
				memoryBarrierImage();
				
				imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 0u);
				keepWaiting = false;
			}
		}
	}
	out_fragColor = vec4(0.0);
}