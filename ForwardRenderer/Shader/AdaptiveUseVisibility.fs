layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

#define STORAGE_READ_ONLY
#include "AdaptiveStorage.glsl"

// the function is unsorted for the unsorted techniques if it was not sorted in resolve
#ifndef UNSORTED_SORT_RESOLVE
#ifdef UNSORTED_LIST
#define UNSORTED_VIS
#endif
#ifdef USE_UNSORTED_HEIGHTS
#define UNSORTED_VIS
#endif
#endif

#ifdef UNSORTED_VIS

float visz(float depth)
{
	// accumulate alpha of all previous fragments
	float alpha = 1.0;
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		vec2 val = LOAD(i);
		if(depth > val.x)
			alpha *= val.y;
	}
	return alpha;
}

#else

float visz(float depth)
{
	float previousTransmittance = 1.0;
	
	int maxZ = MAX_SAMPLES;
	for(int i = 0; i < maxZ; ++i)
	{
		vec2 val = LOAD(i).xy;
		if ( depth <= val.x )
			return previousTransmittance;
		
		previousTransmittance = val.y;
	}
	return previousTransmittance;
	
	/*float previousTransmittance = 1.0;
	vec2 fragments[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
		fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(depth <= fragments[i].x)
			return previousTransmittance;
			
		previousTransmittance = fragments[i].y;
	}
	return previousTransmittance;*/
}

#endif

void main()
{
	vec3 color = calcMaterialColor();
	float dissolve = calcMaterialAlpha();
	
	// determine the occlusion
	float dist = distance(u_cameraPosition, in_position);
	float occlusion = visz(dist);
	
	out_fragColor = vec4(occlusion * dissolve * color, 1.0);
}