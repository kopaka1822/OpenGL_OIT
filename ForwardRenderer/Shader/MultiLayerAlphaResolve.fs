#include "MultiLayerAlphaSettings.glsl"
#define STORAGE_READ_ONLY
#include "MultiLayerAlphaStorage.glsl"

out vec4 out_fragColor;

void main()
{
	// merge all colors
	float mergedAlpha = 1.0;
	vec3 mergedColor = vec3(0.0);
	
	int size = MAX_SAMPLES;
	
#ifdef STORE_UNSORTED
	vec2 fragments[MAX_SAMPLES_C];
	// load function
	for(int i = 0; i < size; ++i)
		fragments[i] = LOAD(i);
	
	// insertion sort (faster)
	/*for(int i = 1; i < size; ++i)
	{
		vec2 cur = fragments[i];
#pragma optionNV (unroll all)
		int j = i;
		for(; j > 0 && fragments[j - 1].x > cur.x; --j)
		{
			fragments[j] = fragments[j - 1];
		}
		

		for(int k = 0; k < size; ++k)
			if( k == j)
				fragments[k] = cur;
#pragma optionNV (unroll)
	}*/
	
	/*for(int i = 1; i < size; ++i)
	{
		vec2 cur = fragments[i];
#pragma optionNV (unroll all)
		int j = i;
		int m = j;
		for(; j > 0; --j)
		{
			if(fragments[j - 1].x > cur.x)
			{
				fragments[j] = fragments[j - 1];
				--m;
			}
		}
		

		for(int k = 0; k < size; ++k)
			if( k == m)
				fragments[k] = cur;
#pragma optionNV (unroll)
	}*/

	// modified insertion sort
	for(int i = 1; i < size; ++i)
	{
		// i - 1 elements are sorted
#pragma optionNV (unroll all)	
		for(int j = i; j > 0 && fragments[j - 1].x > fragments[j].x; --j)
		{
			vec2 tmp = fragments[j];
			fragments[j] = fragments[j - 1];
			fragments[j - 1] = tmp;
		}
#pragma optionNV (unroll)
	}
	
	
	
	// bubble sort
/*#pragma optionNV (unroll all)
	for(int n = size; n > 1; --n)
	{
		bool swapped = true;
		for(int i = 0; i < n - 1; ++i)
		{
			if(fragments[i].x > fragments[i + 1].x)
			{
				vec2 tmp = fragments[i];
				fragments[i] = fragments[i + 1];
				fragments[i + 1] = tmp;
				swapped = false;
			}
		}
		if(swapped) break;
	}
#pragma optionNV (unroll)
*/	
	// now blend together
	for(int i = 0; i < size; ++i)
	{
		vec4 color = unpackColor(fragments[i].y);
		mergedColor += mergedAlpha * color.rgb;
		mergedAlpha *= color.a;
	}
	
#else // sorted
	for(int i = 0; i < size; ++i)
	{
		vec4 color = unpackColor(LOAD(i).y);
		mergedColor += mergedAlpha * color.rgb;
		mergedAlpha *= color.a;
	}
#endif
	// blending is GL_ONE, GL_SRC_ALPHA
	out_fragColor = vec4(mergedColor, mergedAlpha);
}