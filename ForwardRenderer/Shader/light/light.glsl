#include "../uniforms/transform.glsl"
#include "../uniforms/material.glsl"

layout(binding = 1) uniform sampler2D tex_dissolve;
layout(binding = 2) uniform sampler2D tex_diffuse;

#ifndef LIGHT_ONLY_TRANSPARENT

layout(binding = 0) uniform sampler2D tex_ambient;
layout(binding = 3) uniform sampler2D tex_specular;

#include "../uniforms/lights.glsl"

#endif

float calcMaterialAlpha()
{
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
			
			// diffuse angle
			
			
			// angle for specular color
			float hDotN = dot(normalize(-viewDir - direction), normal);
			
			color += 
				max(vec3(0.0), diffuse_col * lightColor * cosTheta) +
				max(vec3(0.0), specular_col * lightColor * pow(max(0.0, hDotN), m_specular.a));
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
	
	return toGamma(color);
}

#endif
