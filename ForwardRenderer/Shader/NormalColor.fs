layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

void main()
{
#ifdef DEBUG_NORMAL
	out_fragColor = vec4((in_normal + vec3(1.0)) / vec3(2.0), 1.0);
#else
#ifdef DEBUG_MESH
	out_fragColor = vec4(0.0);
#else
#ifdef DEBUG_DEPTH
	out_fragColor = vec4(gl_FragCoord.z);
#else
	out_fragColor = vec4(in_texcoord, 1.0, 1.0);
#endif
#endif
#endif
}