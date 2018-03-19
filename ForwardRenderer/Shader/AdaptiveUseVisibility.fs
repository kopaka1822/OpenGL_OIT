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

layout(binding = 7) uniform sampler3D tex_visz;

float visz(float depth)
{
	float previousTransmittance = 1.0;
	
	int maxZ = textureSize(tex_visz, 0).z;
	for(int i = 0; i < maxZ; ++i)
	{
		vec2 val = texelFetch(tex_visz, ivec3(gl_FragCoord.xy, i), 0).xy;
		if ( depth <= val.x )
			return previousTransmittance;
		
		previousTransmittance = val.y;
	}
	return previousTransmittance;
}

//uniform vec3 LIGHT_DIR = vec3(0.267261242, 0.801783726, 0.534522484);

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
	
	// determine the occlusion
	float dist = distance(u_cameraPosition, in_position);
	float occlusion = visz(dist);
	
	out_fragColor = vec4(occlusion * dissolve * color, 1.0);
}