#version 430

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

layout(binding = 0) uniform ubo_transform
{
	mat4 u_model;
	mat4 u_viewProjection;
	vec3 u_cameraPosition;
};

layout(binding = 1) uniform ubo_material
{
	vec3 m_ambient;
	float m_dissolve;
	vec4 m_diffuse;
	vec4 m_specular;
};

layout(binding = 0) uniform sampler2D tex_ambient;
layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;
layout(binding = 3) uniform sampler2D tex_specular;

//uniform vec3 LIGHT_DIR = vec3(0.267261242, 0.801783726, 0.534522484);

void main()
{
	vec3 LIGHT_DIR = normalize(u_cameraPosition - in_position);

	vec3 ambient_col = m_ambient.rgb * texture(tex_ambient, in_texcoord).rgb;
	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	vec3 diffuse_col = m_diffuse.rgb * texture(tex_diffuse, in_texcoord).rgb;
	vec3 specular_col = m_specular.rgb * texture(tex_specular, in_texcoord).rgb;

	vec3 normal = normalize(in_normal);
	
	// angle for diffuse
	float cosTheta = dot(LIGHT_DIR, normal);
	// angle for specular color
	vec3 viewDir = normalize(u_cameraPosition - in_position);
	float HdotN = dot(normalize(viewDir + LIGHT_DIR), normal);
	
	vec3 color = 
		max(vec3(0.0), ambient_col) + // ambient color
		max(vec3(0.0), diffuse_col * cosTheta) + // diffuse color
		max(vec3(0.0), specular_col * pow(max(0.0, HdotN), 6.0)) +
		diffuse_col * 0.01;
	
	out_fragColor = vec4(color, dissolve);
	
	/*
	vec3 normal = normalize(in_normal);
	float cosTheta = dot(LIGHT_DIR, normal);
	float diffuse = cosTheta * 0.5 + 0.5;
	
	vec3 viewDir = normalize(u_cameraPosition - in_position);
	float HdotN = dot(normalize(viewDir + LIGHT_DIR), normal);
	float specular = pow(max(0.0, HdotN), 6.0);
	
	out_fragColor = vec4(vec3(0.7 * diffuse + 0.3 * specular),1.0);
	*/
}