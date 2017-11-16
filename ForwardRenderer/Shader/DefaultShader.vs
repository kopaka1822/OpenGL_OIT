#version 430

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(binding = 0) uniform ubo_transform
{
	mat4 u_model;
	mat4 u_viewProjection;
	vec3 u_cameraPosition;
};

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;

void main()
{
	out_position = (u_model * vec4(in_position, 1.0)).xyz;
	out_normal = (u_model * vec4(in_normal, 0.0)).xyz;
	out_texcoord = in_texcoord;

	gl_Position = u_viewProjection * u_model * vec4(in_position, 1.0);
}