#version 440 core
// generating flat normals for each triangle

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 in_position[3];
layout(location = 1) in vec3 in_normal[3];
layout(location = 2) in vec2 in_texcoord[3];

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texcoord;

layout(binding = 0) uniform ubo_transform
{
	mat4 u_model;
	mat4 u_viewProjection;
	vec3 u_cameraPosition;
	uint u_screenWidth;
};

void main()
{
	bool renormal = false;
	for(int i = 0; i < 3; ++i)
		if(length(in_normal[i]) < 0.001)
			renormal = true;
	
	if(renormal)
	{
		// calculate surface normal
		vec3 normal = normalize(cross(in_position[1] - in_position[0], in_position[2] - in_position[0]));
		
		// flip normal if facing away
		vec3 midpoint = (in_position[0] + in_position[1] + in_position[2]) / 3.0;
		vec3 viewDir = u_cameraPosition - midpoint;
		if(dot(viewDir, normal) < 0)
			normal = -normal;
		
		for(int i = 0; i < 3; ++i)
		{
			out_position = in_position[i];
			out_normal = normal;
			out_texcoord = in_texcoord[i];
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
	}
	else
	{
		// pass through
		for(int i = 0; i < 3; ++i)
		{
			out_position = in_position[i];
			vec3 normal = in_normal[i];
			if(dot(normal, /*view dir*/ u_cameraPosition - in_position[i]) < 0)
				normal = -normal;
				
			out_normal = normal;
			out_texcoord = in_texcoord[i];
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
	}
}