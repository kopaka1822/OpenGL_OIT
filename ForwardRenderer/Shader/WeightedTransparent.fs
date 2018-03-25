layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor0;
layout(location = 1) out float out_fragColor1;

#include "light/light.glsl"

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
	float dissolve = calcMaterialAlpha();
	vec3 color = calcMaterialColor();
	
	float dist = distance(in_position, u_cameraPosition);
	float w = weight(dist, dissolve);
	
	out_fragColor0 = vec4(color * dissolve * w, dissolve);
	out_fragColor1 = w * dissolve;
}