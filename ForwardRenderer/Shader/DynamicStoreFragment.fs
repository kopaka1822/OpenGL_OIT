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

layout(binding = 0) uniform sampler2D tex_ambient;
layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;
layout(binding = 3) uniform sampler2D tex_specular;

layout(binding = 5, std430) coherent buffer ssbo_fragmentCount
{
	uint b_fragmentCount[];
};

layout(binding = 6, std430) readonly buffer ssbo_fragmentBase
{
	uint b_fragmentBase[];
};

struct Fragment
{
	float depth;
	uint color;
};

layout(binding = 7, std430) writeonly buffer ssbo_fragmentStore
{
	 Fragment b_fragmentDest[];
};

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
	
	if(dissolve > 0.0)
	{
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

			// store color etc.
			uint index = uint(gl_FragCoord.y) * u_screenWidth + uint(gl_FragCoord.x);
			uint offset = atomicAdd(b_fragmentCount[index], uint(-1)) - 1;
			uint base = 0;
			if(index > 0)
				base = b_fragmentBase[index - 1];
			
			// store
			uint storeIdx = base + offset;
			
			b_fragmentDest[storeIdx].depth = distance(u_cameraPosition, in_position);
			b_fragmentDest[storeIdx].color = packUnorm4x8(vec4(color, dissolve));
	}
	
	
	out_fragColor = vec4(0.0);
}