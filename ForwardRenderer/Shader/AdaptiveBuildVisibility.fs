layout(early_fragment_tests) in;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_fragColor;

#define LIGHT_ONLY_TRANSPARENT
#include "light/light.glsl"

#include "AdaptiveStorage.glsl"

layout(binding = 1, r32ui) coherent uniform uimage2D tex_atomics;

float getRectArea(vec2 pos1, vec2 pos2)
{
	return (pos2.x - pos1.x) * (pos1.y - pos2.y);
}

#ifdef USE_UNSORTED_HEIGHTS

float g_visExponent = 0.0f;
float g_visOffset = 0.0f;

#define DEPTH(v) ((v).x)
#define ALPHA(v) ((v).y)
#define FLOAT_MAX 3.402823466e+38

float vis(float x)
{
	return exp(g_visExponent * (x + g_visOffset));
}

void insertAlpha(float one_minus_alpha, float depth)
{
	// determine visibility function
	float maxDepth = depth;
	float minDepth = depth;
	float productAlpha = one_minus_alpha;
	float lastAlpha = one_minus_alpha;
	int maxIndex = MAX_SAMPLES;
	int minIndex = MAX_SAMPLES;
	
	vec2 fragments[MAX_SAMPLES + 1];
	fragments[MAX_SAMPLES] = vec2(depth, one_minus_alpha);
	
	// load all fragments
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i] = LOAD(i);
		productAlpha *= ALPHA(fragments[i]);

		if(DEPTH(fragments[i]) > maxDepth)
		{
			// new max depth
			maxDepth = DEPTH(fragments[i]);
			lastAlpha = ALPHA(fragments[i]);
			maxIndex = i;
		}
		
		if(DEPTH(fragments[i]) < minDepth)
		{
			minDepth = DEPTH(fragments[i]);
			minIndex = i;
		}
	}

	// store visibility function

	if(maxDepth == FLOAT_MAX)
	{
		STORE(maxIndex, fragments[MAX_SAMPLES]);
	}
	else
	{
		// avoid dividing by zero
		const float e = 0.00000001;
		g_visExponent = log(max(productAlpha / max(lastAlpha, e), e)) / max(maxDepth - minDepth, e);
		g_visOffset = -minDepth;

		float minHeight = FLOAT_MAX;
		int removeIndex = 0;
		vec2 removeFragment = fragments[0];
	
		// find the node with the smallest height difference (this node will be removed)
		for(int i = 0; i <= MAX_SAMPLES; ++i)
		{
			float height = vis(DEPTH(fragments[i])) * (1.0f - ALPHA(fragments[i]));
			if (height < minHeight && minIndex != i)
			{
				minHeight = height;
				removeIndex = i;
				removeFragment = fragments[i];
			}
		}
		
		// determine the previous node
		int smallestRectIndex = -1;
		vec2 compressFragment;
		DEPTH(compressFragment) = -1.0;

		for(int i = 0; i <= MAX_SAMPLES; ++i)
		{
			if(
				i != removeIndex &&
				DEPTH(compressFragment) <= DEPTH(fragments[i]) &&
				DEPTH(fragments[i]) <= DEPTH(removeFragment) )
			{
				smallestRectIndex = i;
				compressFragment = fragments[i];
			}
		}

		// adjust alpha for compression
		ALPHA(compressFragment) *= ALPHA(removeFragment);

		if(smallestRectIndex == MAX_SAMPLES)
		{
			// only store this
			STORE(removeIndex, compressFragment);
		}
		else
		{
			STORE(smallestRectIndex, compressFragment);
			if(removeIndex != MAX_SAMPLES)
				STORE(removeIndex, fragments[MAX_SAMPLES]);
		}
	}
}

#else // no unsorted heights

#ifdef USE_ARRAY_LINKED_LIST

struct Fragment
{
	float depth;
	float alpha;
	int next;
	int oldPosition;
};

void insertAlpha(float one_minus_alpha, float depth)
{
#pragma optionNV (unroll all)
	// n * (n + 1) and -1 because one link is -1
	/*int currentLink = (MAX_SAMPLES * (MAX_SAMPLES - 1)) / 2 - 1;
	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		Link l = unpackLink(LOAD(i % MAX_SAMPLES));
		currentLink -= l.next;
	}
	//int currentLink = 0;
	
	Fragment fragments[MAX_SAMPLES + 1];
	fragments[0] = Fragment( depth, one_minus_alpha, -1, MAX_SAMPLES );

	// unpack linked list into registers
	for (int i = 1; i <= MAX_SAMPLES; ++i)
	{
		Link l = unpackLink(LOAD(currentLink % MAX_SAMPLES));
		fragments[i] = Fragment( l.depth, l.alpha, l.next, currentLink );
		currentLink = l.next;
	}*/

	//Link links[MAX_SAMPLES];
	int currentLink = (MAX_SAMPLES * (MAX_SAMPLES - 1)) / 2 - 1;
	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		Link l = unpackLink(LOAD(i));
		//links[i] = l;
		currentLink -= l.next;
	}
	
	Fragment fragments[MAX_SAMPLES + 1];
	fragments[0] = Fragment( depth, one_minus_alpha, -1, MAX_SAMPLES );
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		Link l = unpackLink(LOAD(currentLink));//links[currentLink];
		fragments[i+1] =  Fragment( l.depth, l.alpha, l.next, currentLink );
		currentLink = l.next;
	}
	
	// the value that is before the inserted value
	int insertPos = 0;
	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		if (fragments[i].depth > fragments[i + 1].depth)
		{
			const Fragment tmp = fragments[i + 1];
			fragments[i + 1] = fragments[i];
			fragments[i] = tmp;
			insertPos = i + 1;
		}
	}

	// fix links and otain insert and prev insert

	// inserted fragment
	Fragment isFrag = fragments[MAX_SAMPLES];
	// previous to inserted fragment
	Fragment piFrag = fragments[0];;
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i].next = fragments[i + 1].oldPosition;
		if (i + 1 == insertPos)
			piFrag = fragments[i];
		if (i == insertPos)
			isFrag = fragments[i];
	}
	// insertpos == MAX_SAMPLES is handled by isFrag initialization


	// find smallest rectangle
	// find smallest rectangle to insert
	float minRectArea = 1.0 / 0.0;
	int smallestRectPos = 0;
	// smallest rect fragment
	Fragment smFrag = fragments[0];
	// next to smallest rect fragment
	Fragment nsFrag = fragments[1];

	float prevAlpha = 1.0;
	for (int i = 0; i < MAX_SAMPLES; ++i)
	{
		prevAlpha *= fragments[i].alpha;
		float nextAlpha = fragments[i + 1].alpha;
		float area = getRectArea(vec2(fragments[i].depth, prevAlpha),
			vec2(fragments[i + 1].depth, prevAlpha * nextAlpha));
		if (area < minRectArea)
		{
			minRectArea = area;
			smallestRectPos = i;
			smFrag = fragments[i];
			nsFrag = fragments[i + 1];
		}
	}

	// write the (maximum) 3 values who have changed into the array
	Fragment changed[3];
	int numChanged = 1;
	
	// insert the node with modified alpha
	changed[0] = smFrag;
	changed[0].alpha = smFrag.alpha * nsFrag.alpha;
	changed[0].next = nsFrag.next;
	
	if (insertPos == smallestRectPos)
	{
		changed[0].oldPosition = nsFrag.oldPosition;
	}
	else if (insertPos == 0 || insertPos == smallestRectPos + 2)
	{
		// inserted fragment
		changed[1] = isFrag;
		changed[1].oldPosition = nsFrag.oldPosition;
		numChanged = 2;

		if (insertPos == smallestRectPos + 2)
			changed[0].next = nsFrag.oldPosition;
	}
	// insert all 3 nodes
	else if (insertPos != smallestRectPos + 1)
	{
		// inserted node
		changed[1] = isFrag;
		changed[1].oldPosition = nsFrag.oldPosition;

		// node pointing to inserted node
		changed[2] = piFrag;
		changed[2].next = nsFrag.oldPosition;

		numChanged = 3;
	}
	
	
	for(int i = 0; i < 3; ++i)
	{
		if(i < numChanged)
		{
			Fragment val = changed[i];
			STORE(val.oldPosition % MAX_SAMPLES, packLink(Link( val.depth, val.alpha, val.next )));
		}
	}
}

#else // NO ARRAY LINKED LIST
#ifdef UNSORTED_LIST

struct Fragment
{
	float depth;
	float alpha;
	int oldPosition;
};

void insertAlpha(float one_minus_alpha, float depth)
{
	Fragment fragments[MAX_SAMPLES + 1];
	fragments[0] = Fragment(depth, one_minus_alpha, -1);
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		vec2 val = LOAD(i);
		fragments[i + 1] = Fragment(val.x, val.y, i);
	}
	
	// sort values depending on depth
	// modified insertion sort
	for(int i = 1; i <= MAX_SAMPLES; ++i)
	{
		// i - 1 elements are sorted
#pragma optionNV (unroll all)	
		for(int j = i; j > 0 && fragments[j - 1].depth > fragments[j].depth; --j)
		{
			Fragment tmp = fragments[j];
			fragments[j] = fragments[j - 1];
			fragments[j - 1] = tmp;
		}
#pragma optionNV (unroll)
	}

	
	// find smallest rectangle
	// find smallest rectangle to insert
	float minRectArea = 1.0 / 0.0;
	Fragment smallestRectValue = fragments[0];
	Fragment nextSmallestRectValue = fragments[0];
	
	float prevAlpha = 1.0;
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		prevAlpha *= fragments[i].alpha;
		float nextAlpha = fragments[i + 1].alpha;
		float area = getRectArea( vec2(fragments[i].depth, prevAlpha),
								  vec2(fragments[i + 1].depth, prevAlpha * nextAlpha));
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectValue = fragments[i];
			nextSmallestRectValue = fragments[i + 1];
		}
	}
	
	// adjust smallestRectValue
	smallestRectValue.alpha = smallestRectValue.alpha * nextSmallestRectValue.alpha;
	bool storeSecond = true;

	if(nextSmallestRectValue.oldPosition == -1)
	{
		// the inserted value should not be stored
		storeSecond = false;
		// store same value twice
		nextSmallestRectValue = smallestRectValue;
	}
	else if(smallestRectValue.oldPosition == -1)
	{
		// overwrite nextSmallestRectValue.oldPosition
		smallestRectValue.oldPosition = nextSmallestRectValue.oldPosition;
		storeSecond = false;
		nextSmallestRectValue = smallestRectValue;
	} 
	else
	{
		// the inserted value should overwrite the nextSmallestRect
		nextSmallestRectValue.depth = depth;
		nextSmallestRectValue.alpha = one_minus_alpha;
	}
	
	STORE(smallestRectValue.oldPosition,
		vec2(smallestRectValue.depth, smallestRectValue.alpha));
	
	//if(storeSecond)
		STORE(nextSmallestRectValue.oldPosition,
			vec2(nextSmallestRectValue.depth, nextSmallestRectValue.alpha));
}

#else

 // Default
void insertAlphaReference(float one_minus_alpha, float depth)
{ 
	vec2 fragments[MAX_SAMPLES + 1];
	
	// load values Upack AOIT Data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i] = LOAD(i);
	}
	
	int insertPosition = 0;
	float prevAlpha = 1.0;
	
	// find insert index
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(fragments[i].x < depth)
		{
			insertPosition = i + 1;
			prevAlpha = fragments[i].y;
		}

	}
	
	// Make space for the new fragment
	for(int i = MAX_SAMPLES - 1; i >= 0; --i)
	{
		if(insertPosition <= i)
		{
			fragments[i + 1].x = fragments[i].x;
			fragments[i + 1].y = fragments[i].y * one_minus_alpha;
		}
	}
	
	// insert new fragment
	for(int i = 0; i <= MAX_SAMPLES; ++i)
	{
		if(insertPosition == i)
		{
			fragments[i].x = depth;
			fragments[i].y = one_minus_alpha * prevAlpha;
		}
	}
	
	float nodeUnderError[MAX_SAMPLES];
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		nodeUnderError[i] = getRectArea(fragments[i], fragments[i+1]);
	}
	
	// find the node that generates the smallest removal error
	int smallestErrorIndex = 0;
	float smallestError = nodeUnderError[0];
	
	for(int i = 1; i < MAX_SAMPLES; ++i)
	{
		if(nodeUnderError[i] < smallestError)
		{
			smallestError = nodeUnderError[i];
			smallestErrorIndex = i;
		}
	}
	
	// Remove that node
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestErrorIndex < i)
		{
			// adjust depth
			fragments[i].x = fragments[i + 1].x;
		}
	}
	
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestErrorIndex <= i)
		{
			// adjust alpha
			fragments[i].y = fragments[i + 1].y;
		}
	}
	
	// pack aoit data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		STORE(i, fragments[i]);
	}
}

void insertAlpha(float one_minus_alpha, float depth)
{	 
	vec2 fragments[MAX_SAMPLES + 1];
	// load values
	fragments[0] = vec2(depth, one_minus_alpha);
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		fragments[i + 1] = LOAD(i);
	}
	
	// 1 pass bubble sort for new value
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		float newAlpha = fragments[i + 1].y * one_minus_alpha;
		if(fragments[i].x > fragments[i + 1].x)
		{
			// shift lower value and insert new value at i + 1
			fragments[i] = fragments[i + 1];
			fragments[i + 1].x = depth;
			fragments[i + 1].y = newAlpha;
		}
		else
		{
			// adjust values after the insert position
			fragments[i + 1].y = newAlpha;
		}
	}
	
//#define OVERESTIMATE
#define UNDERESTIMATE

	// find smallest rectangle
	// find smallest rectangle to insert
	int smallestRectPos = 0;
	float minRectArea = 1.0 / 0.0;
	
#ifdef UNDERESTIMATE
	// underestimation
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
#ifdef USE_HEIGHT_METRIC
		float area = fragments[i].y - fragments[i+1].y;
		area *= area;
		area /= max(fragments[i].y, 0.0000001);
#else // deafult metric (rectangle area)
		float area = getRectArea(	fragments[i],
									fragments[i+1] );
#endif
									
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectPos = i;
		}
	}
#endif

#ifdef OVERESTIMATE	
	// overestimation
	float prevHeight = 1.0;
	bool overestimated = false;
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		// upper rectangle
		float area = abs((prevHeight - fragments[i].y) * (fragments[i+1].x - fragments[i].x));
		prevHeight = fragments[i].y;
		
		if(area < minRectArea)
		{
			minRectArea = area;
			smallestRectPos = i;
			overestimated = true;
		}
	}
#endif
	
#ifdef UNDERESTIMATE	
#ifdef OVERESTIMATE	
#define BOTH_ESTIMATES
#endif
#endif

#ifdef BOTH_ESTIMATES
	// both, over and understimate
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestRectPos <= i)
		{
			fragments[i].y = fragments[i + 1].y;
		}
		if(smallestRectPos < i || (overestimated && smallestRectPos == i))
		{
			fragments[i] = fragments[i + 1];
		}
	}
#else


#ifdef UNDERESTIMATE
	// understimation adjustment
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestRectPos <= i)
		{
			fragments[i].y = fragments[i + 1].y;
		}
		if(smallestRectPos < i)
		{
			fragments[i] = fragments[i + 1];
		}
	}
#endif

#ifdef OVERESTIMATE	
	// overestimation adjustment
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		if(smallestRectPos <= i)
		{
			fragments[i] = fragments[i+1];
		}
	}
#endif	
#endif

	// pack aoit data
	for(int i = 0; i < MAX_SAMPLES; ++i)
	{
		STORE(i, fragments[i]);
	}
}
#endif // unsorted buffer
#endif // array linked list
#endif // unsorted heights


void main()
{
	float dissolve = calcMaterialAlpha();
	
	float dist = distance(u_cameraPosition, in_position);
	if(dissolve > 0.0 && !gl_HelperInvocation) // is it event visible?
	{
		
		bool keepWaiting = true;
		while(keepWaiting)
		{
			// acquire lock
			if(imageAtomicCompSwap(tex_atomics, ivec2(gl_FragCoord.xy), 0u, 1u) == 0)
			{
				insertAlpha(1.0 - dissolve, dist); 
				memoryBarrier();
				
				imageAtomicExchange(tex_atomics, ivec2(gl_FragCoord.xy), 0u);
				keepWaiting = false;
			}
		}
	}
	
	out_fragColor = vec4(0.7);
}