layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform float farplane;

layout(location = 0) out vec3 viewDir;

void main(void)
{
	vec4 vertex = vec4(0.0, 0.0, 0.0, 1.0);
	if(gl_VertexID == 0u) vertex = vec4(1.0, -1.0, 0.0, 1.0);
	if(gl_VertexID == 1u) vertex = vec4(-1.0, -1.0, 0.0, 1.0);
	if(gl_VertexID == 2u) vertex = vec4(1.0, 1.0, 0.0, 1.0);
	if(gl_VertexID == 3u) vertex = vec4(-1.0, 1.0, 0.0, 1.0);
	gl_Position = vertex;
	viewDir = (transform * vec4(vertex.xy, farplane, 0.0)).xyz;
}