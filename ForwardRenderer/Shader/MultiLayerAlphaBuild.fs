layout(early_fragment_tests) in;

#include "MultiLayerAlphaSettings.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#include "light/light.glsl"
#include "MultiLayerAlphaStorage.glsl"


layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

float packColor(vec4 color)
{
	return uintBitsToFloat(packUnorm4x8(color));
}

vec4 unpackColor(float f)
{
	return unpackUnorm4x8( floatBitsToUint(f) );
}

vec2 merge(vec2 front, vec2 back)
{
	float mergedDepth = front.x;
	vec4 colorFront = unpackColor(front.y);
	vec4 colorBack  = unpackColor(back.y);
	vec3 mergedRgb = colorFront.rgb + colorBack.rgb * colorBack.a;
	float mergedAlpha = colorFront.a * colorBack.a;
	
	return vec2(mergedDepth, packColor(vec4(mergedRgb, mergedAlpha)));
}

// color: color with transmittance instead op alpha (1 - alpha)
// color is also pre multiplied with alpha
void insertFragment(vec4 color, float depth)
{
	int size = MAX_SAMPLES;
	
#ifdef STORE_UNSORTED
	vec2 fragments[MAX_SAMPLES_C + 1];
	vec2 insertedFrag = vec2(depth, packColor(color));
	fragments[MAX_SAMPLES] = insertedFrag;
	
	// load function
	for(int i = 0; i < size; ++i){
		fragments[i] = LOAD(i);
	}
	
	// x = depth, y = color
	vec2 high = vec2(-1.0, 0.0);
	int highIdx = 0;
	// second highest
	vec2 shigh = vec2(-1.0, 0.0);
	int shighIdx = 0;

//#define LCACHE
//#define GLOBAL
#ifdef LCACHE
	
	// find maximum
	for(int i = 0; i <= size; ++i)
	{
		if(fragments[i].x > high.x)
		{
			high = fragments[i];
			highIdx = i;
		}
	}
	high = fragments[highIdx];
	
	// find second highest value
	for(int i = 0; i <= size; ++i)
	{
		if(fragments[i].x > shigh.x && i != highIdx)
		{
			shigh = fragments[i];
			shighIdx = i;
		}
	}
	shigh = fragments[shighIdx];
#else
#ifdef GLOBAL
	for(int i = 0; i < size; ++i)
	{
		if(LOAD(i).x > high.x)
		{
			high = LOAD(i);
			highIdx = i;
		}
	}
	if(insertedFrag.x > high.x)
	{
		high = insertedFrag;
		highIdx = size;
	}
	
	for(int i = 0; i < size; ++i)
	{
		if(LOAD(i).x > shigh.x && i != highIdx)
		{
			shigh = LOAD(i);
			shighIdx = i;
		}
	}
	if(insertedFrag.x > shigh.x && size != highIdx)
	{
		shigh = insertedFrag;
		shighIdx = size;
	}
	
#else // register
	// find maximum
	for(int i = 0; i <= size; ++i)
	{
		if(fragments[i].x > high.x)
		{
			high = fragments[i];
			highIdx = i;
		}
	}
	
	// find second highest value
	for(int i = 0; i <= size; ++i)
	{
		if(fragments[i].x > shigh.x && i != highIdx)
		{
			shigh = fragments[i];
			shighIdx = i;
		}
	}
#endif
#endif 
	
	// merge the two highest fragments
	vec2 merged = merge(shigh, high);

	// is the merged fragment in range?

	if(highIdx >= MAX_SAMPLES)
	{
		// the inserted value was merged immediately
		// just overwrite the second highest value
		highIdx = shighIdx;
		shighIdx = MAX_SAMPLES;
	}

	STORE(highIdx, merged);
	if(shighIdx != MAX_SAMPLES)
	{
		STORE(shighIdx, vec2(depth, packColor(color)));
	}
	

	/*vec2 insertValue = vec2(depth, packColor(color));
	if(highIdx >= MAX_SAMPLES)
	{
		// the inserted value was merged immediately
		// just overwrite the second highest value
		highIdx = shighIdx;
		// store the same value twice
		insertValue = merged;
	}

	STORE(ivec3(gl_FragCoord.xy, highIdx), merged);
	STORE(ivec3(gl_FragCoord.xy, shighIdx), insertValue);
*/
	// merge the two lowest
	/*if(highIdx < MAX_SAMPLES)
	{
		// high index is in range
		STORE(ivec3(gl_FragCoord.xy, highIdx), merged);
		// add the new item?
		if(shighIdx != MAX_SAMPLES) // highIdx != MAX_SAMPLES => new value must be stored
		{
			// this slot is now free
			STORE(ivec3(gl_FragCoord.xy, shighIdx), vec2(depth, packColor(color)));
		}
	}
	else // highIdx == MAX_SAMPLES
	{
		// only shighIdx is in range and the new inserted value was merged into the highest value
		STORE(ivec3(gl_FragCoord.xy, shighIdx), merged);
	}*/
	
#else // Store sorted
	vec2 fragments[MAX_SAMPLES_C + 1];
	
#define INSERTION
#ifdef INSERTION
	// 1 pass insertion sort
	
	vec2 fragment = vec2(depth, packColor(color));
		
	// load function	
	// Version 1 (Standart)	
	//for(int i = 0; i < size; ++i)
	//	fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	//
	//int j = size;
	//for(; j > 0 && fragments[j - 1].x > fragment.x; --j)
	//{
	//	fragments[j] = fragments[j - 1];
	//}
	//fragments[j] = fragment;
	
	// Version 2 (No early out)
	for(int i = 0; i < size; ++i)
		fragments[i] = LOAD(i);
	int j = size;
	int i = j;
	for(; j > 0; --j)
	{
		if(fragments[j - 1].x > fragment.x)
		{
			fragments[j] = fragments[j - 1];
			--i;
		}
	}
	
	//fragments[i] = fragment;
	
	for(int j = 0; j <= size; ++j){
		if(j == i){
			fragments[j] = fragment;
		}
	}
	
	/*if(i < 8){
		if(i < 4){
			if(i < 2) {
				if(i == 0) fragments[0] = fragment;
				else       fragments[1] = fragment;
			}
			else { // i = 2,3
				if(i == 2) fragments[2] = fragment;
				else       fragments[3] = fragment;
			}
		}
		else { // 4,5,6,7
			if(i < 6) {
				if(i == 4) fragments[4] = fragment;
				else       fragments[5] = fragment;
			}
			else { // i = 6,7
				if(i == 6) fragments[6] = fragment;
				else       fragments[7] = fragment;
			}
		}
	}
	else {	
		if(i < 12){
			if(i < 10) {
				if(i == 8) fragments[8] = fragment;
				else       fragments[9] = fragment;
			}
			else { // i = 10, 11
				if(i == 10) fragments[10] = fragment;
				else       fragments[11] = fragment;
			}
		}
		else { // 12,13,14,15
			if(i < 14) {
				if(i == 12) fragments[12] = fragment;
				else       fragments[13] = fragment;
			}
			else { // i = 14, 15
				if(i == 14) fragments[14] = fragment;
				else if(i == 15) fragments[15] = fragment;
				else fragments[16] = fragment;
			}
		}
	}*/
	
	/*switch(i)
	{
		case 0 : fragments[0 ] = fragment;break;
		case 1 : fragments[1 ] = fragment;break;
		case 2 : fragments[2 ] = fragment;break;
		case 3 : fragments[3 ] = fragment;break;
		case 4 : fragments[4 ] = fragment;break;
#if MAX_SAMPLES > 4                       
		case 5 : fragments[5 ] = fragment;break;
		case 6 : fragments[6 ] = fragment;break;
		case 7 : fragments[7 ] = fragment;break;
		case 8 : fragments[8 ] = fragment;break;
#if MAX_SAMPLES > 8                       
		case 9 : fragments[9 ] = fragment;break;
		case 10: fragments[10] = fragment;break;
		case 11: fragments[11] = fragment;break;
		case 12: fragments[12] = fragment;break;
		case 13: fragments[13] = fragment;break;
		case 14: fragments[14] = fragment;break;
		case 15: fragments[15] = fragment;break;
		case 16: fragments[16] = fragment;break;
#if MAX_SAMPLES > 16                      
		case 17: fragments[17] = fragment;break;
		case 18: fragments[18] = fragment;break;
		case 19: fragments[19] = fragment;break;
		case 20: fragments[20] = fragment;break;
		case 21: fragments[21] = fragment;break;
		case 22: fragments[22] = fragment;break;
		case 23: fragments[23] = fragment;break;
		case 24: fragments[24] = fragment;break;
		case 25: fragments[25] = fragment;break;
		case 26: fragments[26] = fragment;break;
		case 27: fragments[27] = fragment;break;
		case 28: fragments[28] = fragment;break;
		case 29: fragments[29] = fragment;break;
		case 30: fragments[30] = fragment;break;
		case 31: fragments[31] = fragment;break;
		case 32: fragments[32] = fragment;break;
#if MAX_SAMPLES > 32                      
		case 33: fragments[33] = fragment;break;
		case 34: fragments[34] = fragment;break;
		case 35: fragments[35] = fragment;break;
		case 36: fragments[36] = fragment;break;
		case 37: fragments[37] = fragment;break;
		case 38: fragments[38] = fragment;break;
		case 39: fragments[39] = fragment;break;
		case 40: fragments[40] = fragment;break;
		case 41: fragments[41] = fragment;break;
		case 42: fragments[42] = fragment;break;
		case 43: fragments[43] = fragment;break;
		case 44: fragments[44] = fragment;break;
		case 45: fragments[45] = fragment;break;
		case 46: fragments[46] = fragment;break;
		case 47: fragments[47] = fragment;break;
		case 48: fragments[48] = fragment;break;
#if MAX_SAMPLES > 48                      
		case 49: fragments[49] = fragment;break;
		case 50: fragments[50] = fragment;break;
		case 51: fragments[51] = fragment;break;
		case 52: fragments[52] = fragment;break;
		case 53: fragments[53] = fragment;break;
		case 54: fragments[54] = fragment;break;
		case 55: fragments[55] = fragment;break;
		case 56: fragments[56] = fragment;break;
		case 57: fragments[57] = fragment;break;
		case 58: fragments[58] = fragment;break;
		case 59: fragments[59] = fragment;break;
		case 60: fragments[60] = fragment;break;
		case 61: fragments[61] = fragment;break;
		case 62: fragments[62] = fragment;break;
		case 63: fragments[63] = fragment;break;
		case 64: fragments[64] = fragment;break;
#endif
#endif
#endif
#endif
#endif
	}
	*/
	// insert at i
	/*for(j = 0; j <= size; ++j)
	{
		if(i == j)
		{
			fragments[j] = fragment;
		}
	}*/
	
	//fragments[i] = fragment;
	
	// Version 3 better conditional move
	//for(int i = 0; i < size; ++i)
	//	fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	//int j = size;
	//int i = j;
	//for(; j > 0; --j)
	//{
	//	bool c = bool(fragments[j - 1].x > fragment.x);
	//	i -= int(c);
	//	if(c)
	//	{
	//		fragments[j] = fragments[j - 1];
	//	}
	//}
	//fragments[i] = fragment;
	//
	// Version 4 2-loop in positive direction
	//for(int i = 0; i < size; ++i)
	//	fragments[i + 1] = LOAD(ivec3(gl_FragCoord.xy, i));
	//
	//int i = 0;
	//for(int j = 0; j < size; ++j)
	//{
	//	if(fragments[j + 1].x <= fragment.x)
	//	{
	//		fragments[j] = fragments[j + 1];
	//		++i;
	//	}
	//}
	//
	////fragments[i] = fragment;
	//
	//for(int j = 0; j <= size; ++j)
	//{
	//	if(i == j)
	//	{
	//		fragments[j] = fragment;
	//	}
	//}
	
	//for(int j = 0; j < size;)
	
	//fragments[i] = fragment;
	//
	// Version 5 3-loop in positive direction
	//for(int i = 0; i < size; ++i)
	//	fragments[i + 1] = LOAD(ivec3(gl_FragCoord.xy, i));
	//
	//int j = 0;
	//int i = j;
	//for(; j < size; ++j)
	//{
	//	bool c = bool(fragments[j + 1].x <= fragment.x);
	//	i += int(c);
	//	if(c)
	//	{
	//		fragments[j] = fragments[j + 1];
	//	}
	//}
	//
	
	//fragments[i] = fragment;
	//for(j = 0; j <= size; ++j)
	//	if(i == j)
	//		fragments[j] = fragment;
	
	// Version 6 wie version 2 mit einem loop

	//for(int i = 0; i < size; ++i)
	//	fragments[i] = LOAD(ivec3(gl_FragCoord.xy, i));
	//	
	//fragments[MAX_SAMPLES] = fragment;
	//
	//int j = size;
	//for(; j > 0; --j)
	//{
	//	if(fragments[j - 1].x > fragment.x)
	//	{
	//		fragments[j] = fragments[j - 1];
	//		fragments[j - 1] = fragment;
	//	}
	//}
	

#else	
	fragments[0] = vec2(depth, packColor(color));
	
	// load function
	for(int i = 0; i < size; ++i){
		fragments[i + 1] = LOAD(i);
	}

	// 1-pass bubble sort to insert fragment
	
	for(int i = 0; i < size; ++i)
	{
		if(fragments[i].x > fragments[i + 1].x)
		{
			// swap
			vec2 temp = fragments[i];
			fragments[i] = fragments[i + 1];
			fragments[i + 1] = temp;
		}
		// list is sorted
		//else break;
	}
#endif
	
	fragments[size - 1] = merge(fragments[size - 1], fragments[size]);
	
	// write back function
	for(int i = 0; i < size; ++i)
		STORE(i, fragments[i]);

#endif
}

void main()
{
	float dissolve = calcMaterialAlpha();
	vec3 color = calcMaterialColor();
	
	float dist = distance(u_cameraPosition, in_position);
	
	if(dissolve > 0.0 && !gl_HelperInvocation) // is it even visible?
	{
		
		bool keepWaiting = true;
		while(keepWaiting)
		{
			// acquire lock
			if(imageAtomicCompSwap(tex_atomics, ivec2(gl_FragCoord.xy), 0u, 1u) == 0)
			{
				insertFragment(vec4(dissolve * color, 1.0 - dissolve), dist);
				
				memoryBarrier();
				imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 0u);
				keepWaiting = false;
			}
		}
	}
	
	out_fragColor = vec4(0.0);
}