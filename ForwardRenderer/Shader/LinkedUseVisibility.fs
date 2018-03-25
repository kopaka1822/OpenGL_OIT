layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

layout(binding = 0, r32ui) readonly uniform uimage2D tex_anchor;

struct BufferData
{
	float invAlpha;
	float depth;
	uint next;
};

layout(binding = 3, std430) readonly buffer buf_visz
{
	BufferData visz_data[];
};

float visz(float depth)
{
	float t = 1.0;

	uint next = imageLoad(tex_anchor, ivec2(gl_FragCoord.xy)).x;
	while(next != 0u)
	{
		// fetch data
		BufferData dat = visz_data[next - 1];
		if(depth > dat.depth)
		{
			// occluded by this fragment
			t *= dat.invAlpha;
		}
		next = dat.next;
	}
	return t;
}

void main()
{
	float dissolve = calcMaterialAlpha();
	vec3 color = calcMaterialColor();
	
	// determine the occlusion
	float dist = distance(u_cameraPosition, in_position);
	float occlusion = visz(dist);
	
	out_fragColor = vec4(occlusion * dissolve * color, 1.0);
}