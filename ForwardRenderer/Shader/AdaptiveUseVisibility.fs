layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

layout(binding = 7) uniform sampler3D tex_visz;

float visz(float depth)
{
	float previousTransmittance = 1.0;
	
	int maxZ = textureSize(tex_visz, 0).z;
	for(int i = 0; i < maxZ; ++i)
	{
		vec2 val = texelFetch(tex_visz, ivec3(gl_FragCoord.xy, i), 0).xy;
		if ( depth <= val.x )
			return previousTransmittance;
		
		previousTransmittance = val.y;
	}
	return previousTransmittance;
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