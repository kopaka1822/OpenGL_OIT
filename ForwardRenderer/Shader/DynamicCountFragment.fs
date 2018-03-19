#version 450

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
	uint u_screenWidth;
};

layout(binding = 1) uniform ubo_material
{
	vec3 m_ambient;
	float m_dissolve;
	vec4 m_diffuse;
	vec4 m_specular;
};

layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;

layout(binding = 5, std430) coherent buffer ssbo_fragmentCount
{
	uint b_fragmentCount[];
};

void main()
{
	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	
	// take the diffuse texture alpha since its sometimes meant to be the alpha
	dissolve *= texture(tex_diffuse, in_texcoord).a;
	
	// count fragment
	uint index = uint(gl_FragCoord.y) * u_screenWidth + uint(gl_FragCoord.x);
	atomicAdd(b_fragmentCount[index], 1);
	
	out_fragColor = vec4(0.0);
}