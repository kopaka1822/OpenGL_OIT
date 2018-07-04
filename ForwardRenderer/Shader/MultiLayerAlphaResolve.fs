#include "MultiLayerAlphaSettings.glsl"
#define STORAGE_READ_ONLY
#include "MultiLayerAlphaStorage.glsl"

out vec4 out_fragColor;

// shell sort data
const int shell_gaps[] = {23, 10, 4, 1};
const int startGap = MAX_SAMPLES > 23 ? 0 : 
					(MAX_SAMPLES > 10 ? 1 : 
					(MAX_SAMPLES > 4 ? 2 : 3));

//const int shell_gaps[] = {20, 9, 4, 1};
//const int startGap = MAX_SAMPLES > 20 ? 0 : 
//					(MAX_SAMPLES > 9 ? 1 : 
//					(MAX_SAMPLES > 4 ? 2 : 3));
const int shell_maxGaps = shell_gaps.length();
					
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

	// shell sort
	/*
#pragma optionNV (unroll all)
	for(int gapIdx = startGap; gapIdx < shell_maxGaps; ++gapIdx)
	{
		const int gap = shell_gaps[gapIdx];
		for(int i = gap; i < size; ++i)
		{
			for(int j = i; j >= gap && fragments[j - gap].x > fragments[j].x; j -= gap)
			{
				vec2 tmp = fragments[j];
				fragments[j] = fragments[j - gap];
				fragments[j - gap] = tmp;
			}
		}
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
	
	// default insertion sort
/*#pragma optionNV (unroll all)
	for(int j = 1; j < size; ++j)
	{
		vec2 key = fragments[j];
		int i = j - 1;
		while(i >= 0 && fragments[i].x > key.x)
		{
			fragments[i+1] = fragments[i];
			--i;
		}
		for(int z = 0; z < j; ++z)
		{
			if(z == i + 1) fragments[z] = key;
		}
		//fragments[i+1] = key;
	}
*/
	
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