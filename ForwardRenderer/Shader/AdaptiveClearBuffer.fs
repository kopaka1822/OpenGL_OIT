layout(location = 0) out vec4 out_fragColor;

#include "AdaptiveStorage.glsl"


void main()
{
	// fill list with empty links
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		STORE(i, packLink(Link(
			3.402823466e+38, // depth (maximum floating point value)
			1.0, // alpha
			i - 1 // next (-1 = end of list)
		)));
	}
	
	out_fragColor = vec4(0.0);
}
