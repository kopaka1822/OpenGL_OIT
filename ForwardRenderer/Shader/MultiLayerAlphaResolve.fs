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
	
	/*// default insertion sort
#pragma optionNV (unroll all)
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
		fragments[i+1] = key;
		switch(i+1){
			case 0: fragments[0] = key; break;
			case 1: fragments[1] = key; break;
			case 2: fragments[2] = key; break;
			case 3: fragments[3] = key; break;
			case 4: fragments[4] = key; break;
			case 5: fragments[5] = key; break;
			case 6: fragments[6] = key; break;
			case 7: fragments[7] = key; break;
			case 8: fragments[8] = key; break;
			case 9: fragments[9] = key; break;
			case 10: fragments[10] = key; break;
			case 11: fragments[11] = key; break;
			case 12: fragments[12] = key; break;
			case 13: fragments[13] = key; break;
			case 14: fragments[14] = key; break;
			case 15: fragments[15] = key; break;
			case 16: fragments[16] = key; break;
			case 17: fragments[17] = key; break;
			case 18: fragments[18] = key; break;
			case 19: fragments[19] = key; break;
			case 20: fragments[20] = key; break;
			case 21: fragments[21] = key; break;
			case 22: fragments[22] = key; break;
			case 23: fragments[23] = key; break;
			case 24: fragments[24] = key; break;
			case 25: fragments[25] = key; break;
			case 26: fragments[26] = key; break;
			case 27: fragments[27] = key; break;
			case 28: fragments[28] = key; break;
			case 29: fragments[29] = key; break;
			case 30: fragments[30] = key; break;
			case 31: fragments[31] = key; break;
		}
		if(i < 15){
			if(i < 7){
				if(i < 3) {
					if(i < 1){
						if(i==-1) 	fragments[0] = key;
						else		fragments[1] = key;
					} else {
						if(i==1)	fragments[2] = key;
						else 		fragments[3] = key;
					}
				} else {
					if(i < 5){
						if(i==3) 	fragments[4] = key;
						else		fragments[5] = key;
					} else {
						if(i==5)	fragments[6] = key;
						else 		fragments[7] = key;
					}
				}
			} else {
				if(i < 11) {
					if(i < 9){
						if(i==7) 	fragments[8] = key;
						else		fragments[9] = key;
					} else {
						if(i==9)	fragments[10] = key;
						else 		fragments[11] = key;
					}
				} else {
					if(i < 13){
						if(i==11) 	fragments[12] = key;
						else		fragments[13] = key;
					} else {
						if(i==13)	fragments[14] = key;
						else 		fragments[15] = key;
					}
				}
			}
		} else {
			if(i < 23){
				if(i < 19) {
					if(i < 17){
						if(i==15) 	fragments[16] = key;
						else		fragments[17] = key;
					} else {
						if(i==17)	fragments[18] = key;
						else 		fragments[19] = key;
					}
				} else {
					if(i < 21){
						if(i==19) 	fragments[20] = key;
						else		fragments[21] = key;
					} else {
						if(i==21)	fragments[22] = key;
						else 		fragments[23] = key;
					}
				}
			} else {
				if(i < 27) {
					if(i < 25){
						if(i==23) 	fragments[24] = key;
						else		fragments[25] = key;
					} else {
						if(i==25)	fragments[26] = key;
						else 		fragments[27] = key;
					}
				} else {
					if(i < 29){
						if(i==27) 	fragments[28] = key;
						else		fragments[29] = key;
					} else {
						if(i==29)	fragments[30] = key;
						else 		fragments[31] = key;
					}
				}
			}
		}
	}*/

	
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