#version 430

layout(location = 0) in int in_positionIndex;
layout(location = 1) in int in_normalIndex;
layout(location = 2) in int in_texcoordIndex;

layout(binding = 0) uniform ubo_transform
{
	mat4 u_model;
	mat4 u_viewProjection;
	vec3 u_cameraPosition;
};

layout(binding = 0, std430) buffer ssbo_positions
{
	float[] b_positions;
};

layout(binding = 1, std430) buffer ssbo_normals
{
	float[] b_normals;
};

layout(binding = 2, std430) buffer ssbo_texCoords
{
	float[] b_texCoords;
};

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;

void main()
{
	vec3 position = vec3(
		b_positions[in_positionIndex * 3],
		b_positions[in_positionIndex * 3 + 1],
		b_positions[in_positionIndex * 3 + 2]);

	vec3 normal = vec3(0.0);
	if(in_normalIndex != -1)
		normal = vec3(
			b_normals[in_normalIndex * 3],
			b_normals[in_normalIndex * 3 + 1],
			b_normals[in_normalIndex * 3 + 2]);
		
	vec2 texcoord = vec2(0.0);
	if(in_texcoordIndex != -1)
		texcoord = vec2( b_texCoords[in_texcoordIndex * 2],
			b_texCoords[in_texcoordIndex * 2 + 1]);
	
	out_position = (u_model * vec4(position, 1.0)).xyz;
	out_normal = (u_model * vec4(normal, 0.0)).xyz;
	out_texcoord = texcoord;

	gl_Position = u_viewProjection * u_model * vec4(position, 1.0);
}