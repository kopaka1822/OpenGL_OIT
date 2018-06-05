#include "uniforms/transform.glsl"

layout(location = 0) in vec4 in_fragPos;

layout(location = 0) uniform vec3 lightPos;

void main()
{
	float lightDistance = length(in_fragPos.xyz - lightPos);
	
	// map to [0, 1]
	lightDistance /= u_farPlane;
	gl_FragDepth = lightDistance;
	
}