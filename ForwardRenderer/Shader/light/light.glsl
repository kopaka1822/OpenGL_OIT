#include "../uniforms/transform.glsl"
#include "../uniforms/material.glsl"

layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;

#ifndef LIGHT_ONLY_TRANSPARENT

layout(binding = 0) uniform sampler2D tex_ambient;
layout(binding = 3) uniform sampler2D tex_specular;

#ifndef DISABLE_ENVIRONMENT
layout(binding = 8) uniform samplerCube tex_environment;
#endif

#include "../uniforms/lights.glsl"

#endif

float calcMaterialAlpha()
{
	//return 0.9;
	float dissolve = m_dissolve * texture(tex_dissolve, in_texcoord).r;
	// take the diffuse texture alpha since its sometimes meant to be the alpha
	return dissolve * texture(tex_diffuse, in_texcoord).a;
}

#ifndef LIGHT_ONLY_TRANSPARENT

vec3 toGamma(vec3 vec)
{
	//return vec;
	return pow(vec, vec3(1.0 / 2.2));
}

vec3 fromGamma(vec3 vec)
{
	//return vec;
	return pow(vec, vec3(2.2));
}

vec3 calcMaterialColor()
{
	vec3 LIGHT_DIR = normalize(in_position - u_cameraPosition);

	vec3 ambient_col  = fromGamma( m_ambient.rgb  * texture(tex_ambient, in_texcoord).rgb);
	vec3 diffuse_col  = fromGamma( m_diffuse.rgb  * texture(tex_diffuse, in_texcoord).rgb);
	vec3 specular_col = fromGamma( m_specular.rgb * texture(tex_specular, in_texcoord).rgb);
	
	vec3 normal = normalize(in_normal);
	
	const vec3 viewDir = normalize(in_position - u_cameraPosition);
	
	vec3 color = vec3(0.0);
	
	int numLights = min(NUM_LIGHTS.x, 8);
	if(numLights > 0)
	{
		for(int i = 0; i < numLights; ++i)
		{
			vec3 direction;
			vec3 lightColor = lights[i].color.rgb;
			// calculate lightning
			float cosTheta;
			
			if(lights[i].position.w == 1.0f)
			{
				// calculate attenuation
				float dist = distance(in_position, lights[i].position.xyz);
				
				// point light
				direction = (in_position - lights[i].position.xyz) / dist;
				
				lightColor *= 1.0 / (1.0 + lights[i].attenuation.x * dist + lights[i].attenuation.y * dist * dist);
				
				cosTheta = dot(-direction, normal);
			}
			else
			{
				// directional light
				direction = normalize(lights[i].position.xyz);
				
				cosTheta = dot(-direction, normal) * 0.5 + 0.5;
				//cosTheta *= cosTheta;
			}
			
			// diffuse
			if(m_illum != 3)
				color += max(vec3(0.0), diffuse_col * lightColor * cosTheta);
			
			// angle for specular color
			float hDotN = dot(normalize(-viewDir - direction), normal);
			float phongExponent = pow(max(0.0, hDotN), m_specular.a);
			
			if(m_illum != 0 && m_illum != 1)
			{
				// phong specular
				
				color += max(vec3(0.0), specular_col * lightColor * phongExponent);
			}
		}
	}
	else
	{
		float cosTheta = dot(-LIGHT_DIR, normal);
		// angle for specular color
		vec3 viewDir = normalize(in_position - u_cameraPosition);
		float HdotN = dot(normalize(-viewDir - LIGHT_DIR), normal);
		
		color = 
			max(vec3(0.0), ambient_col) + // ambient color
			max(vec3(0.0), diffuse_col * cosTheta) + // diffuse color
			max(vec3(0.0), specular_col * pow(max(0.0, HdotN), m_specular.a)) +
			diffuse_col * 0.01;
	}
	
#ifndef DISABLE_ENVIRONMENT
	if(m_illum == 3 || m_illum == 4)
	{
		// calculate reflected for specular
		vec3 reflected = reflect(viewDir, normal);
		//vec3 refColor = max(vec3(0.0), texture(tex_environment, reflected, m_roughness * 8.0).rgb) * specular_col;
		vec3 refColor = max(vec3(0.0), texture(tex_environment, reflected, m_roughness * 8.0).rgb);
		float lum = clamp(dot(refColor, vec3(1, 1, 1)), 0.0, 1.0);
		color += refColor * pow(lum, 2) * 20.0 * specular_col;
		//color += refColor * pow(lum, 2) * 20.0;
	}
#endif
	
#ifndef DISABLE_ENVIRONMENT
	return clamp(toGamma(color), vec3(0.0), vec3(1.0));
#else
	return clamp(color, vec3(0.0), vec3(1.0));
#endif
}

#endif
