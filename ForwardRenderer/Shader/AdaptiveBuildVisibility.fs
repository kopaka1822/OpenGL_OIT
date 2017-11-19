#version 440

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

layout(binding = 6, r32ui) coherent uniform uimage2D tex_atomics;
layout(binding = 4, rg32f) uniform image3D tex_visz;
layout(std430, binding = 5) buffer atomic_buffer
{
	uint b_imageWidth;
	uint b_atomics[];
};

layout(pixel_center_integer) in vec4 gl_FragCoord;

void main()
{
	uint index = (uint(gl_FragCoord.y) * b_imageWidth + uint(gl_FragCoord.x));// % 1200u;
	vec4 color = vec4(0.0);
	
	uint ret = atomicExchange(b_atomics[index], 1u);
	if(ret == 0u)
	{
		color = vec4(0.0, 1.0, 0.0, 1.0);
		atomicExchange(b_atomics[index], 0u);
	}
	else{
		color = vec4(1.0, 0.0, 0.0, 1.0);
	}
	out_fragColor = color;
	return;
	//atomicAdd(b_atomics[index], uint(-1));
	//uint ret = 0u;
	//uint ret = atomicExchange(b_atomics[index], 1u);
	if(ret == 0u)
		out_fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	else
		out_fragColor = vec4(float(ret) * 0.25);
}