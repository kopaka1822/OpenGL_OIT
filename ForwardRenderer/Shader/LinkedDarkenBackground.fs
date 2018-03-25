layout(binding = 0, r32ui) readonly uniform uimage2D tex_anchor;

struct BufferData
{
	float invAlpha;
	float depth;
	uint next;
};

layout(binding = 3, std430) readonly buffer buf_visz
{
	BufferData visz_data[];
};

float visz()
{
	float t = 1.0;

	uint next = imageLoad(tex_anchor, ivec2(gl_FragCoord.xy)).x;
	while(next != 0u)
	{
		// fetch data
		BufferData dat = visz_data[next - 1];
		t *= dat.invAlpha;
		next = dat.next;
	}
	return t;
}

out vec4 out_fragColor;

void main()
{
	float alpha = visz();

	out_fragColor = vec4(0.0, 0.0, 0.0, alpha);
}