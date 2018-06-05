layout(location = 0) in int in_positionIndex;

#include "uniforms/transform.glsl"

// texture 0 - 3 occupied by fragment shader

layout(binding = 4) uniform samplerBuffer buf_positions;

layout(location = 0) out vec4 out_fragPos;

void main()
{
	vec3 position = vec3(texelFetch(buf_positions, in_positionIndex));

	out_fragPos = u_model * vec4(position, 1.0);
	gl_Position = u_viewProjection * u_model * vec4(position, 1.0);
}