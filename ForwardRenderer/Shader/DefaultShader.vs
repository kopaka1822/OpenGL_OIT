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

// texture 0 - 3 occupied by fragment shader

layout(binding = 4) uniform samplerBuffer buf_positions;
layout(binding = 5) uniform samplerBuffer buf_normals;
layout(binding = 6) uniform samplerBuffer buf_texCoords;

/*layout(binding = 0, std430) buffer ssbo_positions
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
};*/

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;

void main()
{
	vec3 position = vec3(texelFetch(buf_positions, in_positionIndex));

	vec3 normal = vec3(0.0);
	if(in_normalIndex != -1)
		normal = vec3(texelFetch(buf_normals, in_normalIndex));
		
	vec2 texcoord = vec2(0.0);
	if(in_texcoordIndex != -1)
		texcoord = vec2(texelFetch(buf_texCoords, in_texcoordIndex));
	
	out_position = (u_model * vec4(position, 1.0)).xyz;
	out_normal = (u_model * vec4(normal, 0.0)).xyz;
	out_texcoord = texcoord;

	gl_Position = u_viewProjection * u_model * vec4(position, 1.0);
}