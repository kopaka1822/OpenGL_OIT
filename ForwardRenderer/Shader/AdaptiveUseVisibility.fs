layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

// visibility function (xy = fragment xy, z = depth index)
#ifdef SSBO_STORAGE

#include "uniforms/transform.glsl"
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

float visz(float depth)
{
	float previousTransmittance = 1.0;
	
	int maxZ = MAX_SAMPLES;
	for(int i = 0; i < maxZ; ++i)
	{
		vec2 val = LOAD(ivec3(gl_FragCoord.xy, i)).xy;
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

void main()
{
	vec3 color = calcMaterialColor();
	float dissolve = calcMaterialAlpha();
	
	// determine the occlusion
	float dist = distance(u_cameraPosition, in_position);
	float occlusion = visz(dist);
	
	out_fragColor = vec4(occlusion * dissolve * color, 1.0);
}