
struct LightData
{
	// point light (if w = 1.0)
	// directional light (if w = 0.0)
	vec4 position;
	
	vec4 color;
	
	// x = linear, y = quadratic
	vec4 attenuation;
};

layout(binding = 2, std140) uniform ubo_lights
{
	ivec4 NUM_LIGHTS;
	
	LightData lights[8];
};