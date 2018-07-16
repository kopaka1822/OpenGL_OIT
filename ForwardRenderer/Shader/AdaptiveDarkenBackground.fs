layout(location = 0) out vec4 out_fragColor;
layout(early_fragment_tests) in;

#include "uniforms/transform.glsl"

// read only access for the default technique
#ifndef UNSORTED_LIST
#ifndef USE_ARRAY_LINKED_LIST
#ifndef USE_UNSORTED_HEIGHTS
#define STORAGE_READ_ONLY
#endif
#endif
#endif

// read only access for unsorted if nothing is sorted in resolve
#ifndef UNSORTED_SORT_RESOLVE
#ifdef UNSORTED_LIST
#define STORAGE_READ_ONLY
#endif
#ifdef USE_UNSORTED_HEIGHTS
#define STORAGE_READ_ONLY
#endif
#endif


#include "AdaptiveStorage.glsl"

// use a simple depth sort for these two techniques
#ifdef UNSORTED_LIST
#define SIMPLE_SORT_LIST
#endif
#ifdef USE_UNSORTED_HEIGHTS
#define SIMPLE_SORT_LIST
#endif

#ifndef USE_ARRAY_LINKED_LIST
#ifndef SIMPLE_SORT_LIST

void main()
{
	
	int maxZ = MAX_SAMPLES;
	float transmittance = 
		LOAD(maxZ - 1).y;

	out_fragColor = vec4(0.0, 0.0, 0.0, transmittance);
}

#else // unsorted list and unsorted heights

#ifdef UNSORTED_SORT_RESOLVE
// sort list in resolve stage (faster read for second geometry pass)

void main()
{
	// load and sort function
	vec2 fragments[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
		fragments[i] = LOAD(i);
		
	// sort values depending on depth
	// modified insertion sort
#pragma optionNV (unroll all)	
	for(int i = 1; i < MAX_SAMPLES; ++i)
	{
		// i - 1 elements are sorted
		for(int j = i; j > 0 && fragments[j - 1].x > fragments[j].x; --j)
		{
			vec2 tmp = fragments[j];
			fragments[j] = fragments[j - 1];
			fragments[j - 1] = tmp;
		}
	}
	
	float prevAlpha = 1.0;
	// accumulate alpha
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		prevAlpha *= fragments[i].y;
		fragments[i].y = prevAlpha;
	}
	
	// store values
	for(int i = 0; i < MAX_SAMPLES; ++i)
		STORE(i, fragments[i]);
		
	out_fragColor = vec4(0.0, 0.0, 0.0, fragments[MAX_SAMPLES - 1].y);
}

#else // unsorted -> dont sort in resolve stage

void main()
{
	// accumulate alpha values
	float alpha = 1.0;
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		alpha *= LOAD(i).y;
	}
	
	out_fragColor = vec4(0.0, 0.0, 0.0, alpha);
}

#endif // unsorted sort resolve

#endif

#else // array linked list

// for now just sort again
// TODO just resolve linked list
void main()
{
	// load and sort function
	vec2 fragments[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
		fragments[i] = LOAD(i);
		
	// fix alpha values (they are mixed with the next pointer)
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i].y = unpackLink(fragments[i]).alpha;
	}
	
	// TODO dont sort -> use the linked list instead
	// sort values depending on depth
	// modified insertion sort
	for(int i = 1; i < MAX_SAMPLES; ++i)
	{
		// i - 1 elements are sorted
#pragma optionNV (unroll all)	
		for(int j = i; j > 0 && fragments[j - 1].x > fragments[j].x; --j)
		{
			vec2 tmp = fragments[j];
			fragments[j] = fragments[j - 1];
			fragments[j - 1] = tmp;
		}
	}
	
	float prevAlpha = 1.0;
	// accumulate alpha
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		prevAlpha *= fragments[i].y;
		fragments[i].y = prevAlpha;
	}
	
	// store values
	for(int i = 0; i < MAX_SAMPLES; ++i)
		STORE(i, fragments[i]);
		
	out_fragColor = vec4(0.0, 0.0, 0.0, fragments[MAX_SAMPLES - 1].y);
}

#endif
