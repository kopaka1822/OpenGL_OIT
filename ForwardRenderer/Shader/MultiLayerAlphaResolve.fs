#version 430 core

layout(binding = 7) uniform sampler3D tex_fragments; // .x = depth, .y = color (rgba as uint)

vec4 unpackColor(float f)
{
	return unpackUnorm4x8(floatBitsToUint(f));
}

out vec4 out_fragColor;

void main()
{
	// merge all colors
	float mergedAlpha = 1.0;
	vec3 mergedColor = vec3(0.0);
	
	int size = textureSize(tex_fragments, 0).z;
	for(int i = 0; i < size; ++i)
	{
		vec4 color = unpackColor(texelFetch(tex_fragments, ivec3(gl_FragCoord.xy, i), 0).y);
		mergedColor += mergedAlpha * color.rgb;
		mergedAlpha *= color.a;
	}
	
	// blending is GL_ONE, GL_SRC_ALPHA
	out_fragColor = vec4(mergedColor, mergedAlpha);
}