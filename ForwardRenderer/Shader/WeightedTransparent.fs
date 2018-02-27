#version 430

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor0;
layout(location = 1) out float out_fragColor1;

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

float getTransWeight(float z, float a)
{
	return a * pow(z,4.0);
}

float clampZ(float z)
{
	// map should be from -0.5 to 31.5
	// linear clamp from ~ -10 to 100 to be sure
	
	return 1.0 - pow((z + 16.0) / 128.0, 4.0);
}

float weight(float z, float alpha)
{
	//return 1.0;
	//return alpha * max(10e-2, min(3e3, 0.03 / (10e-5 + pow(abs(z) / 200.0, 4.0))));
	//return alpha * max(10e-2, min(3e3, 10.0 / (10e-5 + pow(abs(z) * 0.1, 3.0) + pow(abs(z) / 200.0, 6.0))));
	return alpha * pow(z, -4.0);
	//return 300000 * pow(1.3, -abs(z)); // village best
}

void main()
{
	vec3 LIGHT_DIR = normalize(in_position - u_cameraPosition);

	vec3 ambient_col = m_ambient.rgb * texture(tex_ambient, in_texcoord).rgb;
	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	vec3 diffuse_col = m_diffuse.rgb * texture(tex_diffuse, in_texcoord).rgb;
	vec3 specular_col = m_specular.rgb * texture(tex_specular, in_texcoord).rgb;
	
	// take the diffuse texture alpha since its sometimes meant to be the alpha
	dissolve *= texture(tex_diffuse, in_texcoord).a;
	
	vec3 normal = normalize(in_normal);
	
	// angle for diffuse
	float cosTheta = dot(-LIGHT_DIR, normal);
	// angle for specular color
	vec3 viewDir = normalize(in_position - u_cameraPosition);
	float HdotN = dot(normalize(viewDir + LIGHT_DIR), normal);
	
	vec3 color = 
		max(vec3(0.0), ambient_col) + // ambient color
		max(vec3(0.0), diffuse_col * cosTheta) + // diffuse color
		max(vec3(0.0), specular_col * pow(max(0.0, -HdotN), m_specular.a)) +
		diffuse_col * 0.01;
	
	float dist = distance(in_position, u_cameraPosition);
	float w = weight(dist, dissolve);
	
	out_fragColor0 = vec4(color * dissolve * w, dissolve);
	out_fragColor1 = w * dissolve;
}