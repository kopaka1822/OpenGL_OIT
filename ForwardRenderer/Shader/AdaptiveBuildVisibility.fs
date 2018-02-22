#version 450 core

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

// visibility function (xy = fragment xy, z = depth index)
layout(binding = 0, rg32f) coherent uniform image3D tex_visz; // .x = depth, .y = transmittance
layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

// globals to avoid passing arguments every time
float g_insertedDepth;
float g_insertedValue;
int g_insertPos;
vec2 g_oldFunction[MAX_SAMPLES];

float getViszDepthValue(int position)
{
	return g_oldFunction[position].x;
}

vec2 getNewViszValue(int position)
{
	if(position < g_insertPos)
		return g_oldFunction[position];
	if(position == g_insertPos)
		return vec2(g_insertedDepth, g_insertedValue);
	return g_oldFunction[max(position - 1,0)];
}

float getRectArea(vec2 pos1, vec2 pos2)
{
	return abs(pos2.x - pos1.x) * abs(pos1.y - pos2.y);
}

void loadFunction(int maxZ)
{
	for(int i = 0; i < maxZ; ++i)
	{
		g_oldFunction[i] = imageLoad(tex_visz, ivec3(gl_FragCoord.xy, i)).xy;
	}
}

void insertAlpha(float one_minus_alpha, float depth)
{
	g_insertedDepth = depth;
	
	int maxZ = imageSize(tex_visz).z;
	// get all values from the texture
	loadFunction(maxZ);
	// find point to insert the fragment
	g_insertPos = 0;
	while(g_insertPos != maxZ && getViszDepthValue(g_insertPos) < depth)
	{
		++g_insertPos;
	}
	// g_insertPos [0, maxZ]
	
	// value at g_insertPos ?
	g_insertedValue = 1.0;
	if(g_insertPos >= 1)
		g_insertedValue = g_oldFunction[g_insertPos - 1].y;
		
	g_insertedValue *= one_minus_alpha;
		
	// recalculate function for bigger indices
	for(int i = g_insertPos; i < maxZ; ++i)
	{
		g_oldFunction[i].y *= one_minus_alpha;
	}
	
	// find smallest rectangle to insert
	int smallestRectPos = 0;
	float minRectArea = getRectArea( getNewViszValue(0), getNewViszValue(1) );
	
	for(int i = 1; i < maxZ; ++i)
	{
		float area = getRectArea(	getNewViszValue(i),
									getNewViszValue(i + 1) );
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectPos = i;
		}
	}
	
	// do underestimation (samllestRectPos will get smaller)
	// everything before smallestRectPos will remain the same
	// store the new function
	
	// overwrite everything after and inclusive g_insertPos
	for(int i = g_insertPos; i < smallestRectPos; ++i)
	{
		vec2 value = getNewViszValue(i);
		imageStore(tex_visz, ivec3(gl_FragCoord.xy, i), vec4(value, 0.0, 0.0));
	}
	
	// do the underestimation
	vec2 rectFront = getNewViszValue(smallestRectPos);
	vec2 rectBack = getNewViszValue(smallestRectPos + 1);
	// depth value from first, transmittance from second
	imageStore(tex_visz, ivec3(gl_FragCoord.xy, smallestRectPos), vec4(rectFront.x, rectBack.y, 0.0, 0.0));
	
	// everything after smallestRectPos will be shifted by one to the left
	for(int i = smallestRectPos + 1; i < maxZ; ++i)
	{
		vec2 value = getNewViszValue(i + 1);
		imageStore(tex_visz, ivec3(gl_FragCoord.xy, i), vec4(value, 0.0, 0.0));
	}
	
}

void main()
{

	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	
	// take the diffuse texture alpha since its sometimes meant to be the alpha
	dissolve *= texture(tex_diffuse, in_texcoord).a;
	float dist = distance(u_cameraPosition, in_position);
	if(dissolve >= 0.0 && !gl_HelperInvocation) // is it event visible?
	{
		
		bool keepWaiting = true;
		while(keepWaiting)
		{
			// acquire lock
			if(imageAtomicCompSwap(tex_atomics, ivec2(gl_FragCoord.xy), 0u, 1u) == 0)
			//if(imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 1u) == 0u)
			{
				insertAlpha(1.0 - dissolve, dist); 
				
				memoryBarrier();
				imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 0u);
				keepWaiting = false;
			}
		}
	}
	out_fragColor = vec4(0.0);
}