#version 430

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"

void main()
{
	out_fragColor = vec4(calcMaterialColor(), calcMaterialAlpha());
}