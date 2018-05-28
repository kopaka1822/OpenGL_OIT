layout(binding = 1) uniform ubo_material
{
	vec3 m_ambient;
	float m_dissolve;
	
	vec3 m_diffuse;
	int m_illum;
	
	vec4 m_specular;
	
	vec3 m_transmittance;
	float m_refraction;
	
	float m_roughness;
	float m_metallic;
};