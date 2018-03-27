layout(binding = 7) uniform sampler3D tex_fragments; // .x = depth, .y = color (rgba as uint)

vec4 unpackColor(float f)
{
	return unpackUnorm4x8(floatBitsToUint(f));
}

//#define STORE_UNSORTED

out vec4 out_fragColor;

void main()
{
	// merge all colors
	float mergedAlpha = 1.0;
	vec3 mergedColor = vec3(0.0);
	
	int size = MAX_SAMPLES;
	
#ifdef STORE_UNSORTED
	vec2 fragments[MAX_SAMPLES];
	// load function
	for(int i = 0; i < size; ++i)
		fragments[i] = texelFetch(tex_fragments, ivec3(gl_FragCoord.xy, i), 0).xy;
		
	// sort (insertion sort)
	for(int i = 1; i < size; ++i)
	{
		vec2 cur = fragments[i];
		int j = i;
		for(; j > 0 && fragments[j - 1].x > cur.x; --j)
		{
			fragments[j] = fragments[j - 1];
		}
		fragments[j] = cur;
	}
	
	// now blend together
	for(int i = 0; i < size; ++i)
	{
		vec4 color = unpackColor(fragments[i].y);
		mergedColor += mergedAlpha * color.rgb;
		mergedAlpha *= color.a;
	}
	
#else
	for(int i = 0; i < size; ++i)
	{
		vec4 color = unpackColor(texelFetch(tex_fragments, ivec3(gl_FragCoord.xy, i), 0).y);
		mergedColor += mergedAlpha * color.rgb;
		mergedAlpha *= color.a;
	}
#endif
	// blending is GL_ONE, GL_SRC_ALPHA
	out_fragColor = vec4(mergedColor, mergedAlpha);
}