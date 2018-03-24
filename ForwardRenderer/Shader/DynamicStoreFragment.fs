#version 450

layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

layout(binding = 5, std430) coherent buffer ssbo_fragmentCount
{
	uint b_fragmentCount[];
};

layout(binding = 6, std430) readonly buffer ssbo_fragmentBase
{
	uint b_fragmentBase[];
};

struct Fragment
{
	float depth;
	uint color;
};

layout(binding = 7, std430) writeonly buffer ssbo_fragmentStore
{
	 Fragment b_fragmentDest[];
};

//uniform vec3 LIGHT_DIR = vec3(0.267261242, 0.801783726, 0.534522484);

void main()
{
	float dissolve = calcMaterialAlpha();
	
	if(dissolve > 0.0)
	{
			vec3 color = calcMaterialColor();

			// store color etc.
			uint index = uint(gl_FragCoord.y) * u_screenWidth + uint(gl_FragCoord.x);
			uint offset = atomicAdd(b_fragmentCount[index], uint(-1)) - 1;
			uint base = 0;
			if(index > 0)
				base = b_fragmentBase[index - 1];
			
			// store
			uint storeIdx = base + offset;
			
			b_fragmentDest[storeIdx].depth = distance(u_cameraPosition, in_position);
			b_fragmentDest[storeIdx].color = packUnorm4x8(vec4(color, dissolve));
	}
	
	
	out_fragColor = vec4(0.0);
}