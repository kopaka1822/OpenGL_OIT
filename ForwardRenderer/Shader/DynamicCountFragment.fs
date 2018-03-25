layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#define LIGHT_ONLY_TRANSPARENT
#include "light/light.glsl"

layout(binding = 5, std430) coherent buffer ssbo_fragmentCount
{
	uint b_fragmentCount[];
};

void main()
{
	float dissolve = calcMaterialAlpha();
	
	if(dissolve > 0.0)
	{
		// count fragment
		uint index = uint(gl_FragCoord.y) * u_screenWidth + uint(gl_FragCoord.x);
		atomicAdd(b_fragmentCount[index], 1);
	}
	out_fragColor = vec4(0.0);
}