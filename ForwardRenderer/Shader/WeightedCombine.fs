#version 440 core

layout(binding = 0) uniform sampler2D tex_opaque;
layout(binding = 1) uniform sampler2D tex_transparent1;
layout(binding = 2) uniform sampler2D tex_transparent2;

out vec4 out_color;

void main()
{
	vec4 accum = texelFetch(tex_transparent1, ivec2(gl_FragCoord.xy), 0);
	float r = accum.a;
	accum.a = texelFetch(tex_transparent2, ivec2(gl_FragCoord.xy), 0).r;
	
	vec4 srcColor = vec4(accum.rgb / clamp(accum.a, 1e-4, 5e4), r);
	vec4 dstColor = texelFetch(tex_opaque, ivec2(gl_FragCoord.xy), 0);
	
	out_color = (1.0 - srcColor.a) * srcColor + srcColor.a * dstColor;
}