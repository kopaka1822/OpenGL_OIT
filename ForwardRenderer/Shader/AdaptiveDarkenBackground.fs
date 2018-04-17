layout(location = 0) out vec4 out_fragColor;
layout(binding = 7) uniform sampler3D tex_visz;

void main()
{
	
	int maxZ = MAX_SAMPLES;
	float transmittance = 
		texelFetch(tex_visz, ivec3(gl_FragCoord.xy, maxZ - 1), 0).y;

	out_fragColor = vec4(0.0, 0.0, 0.0, transmittance);
}