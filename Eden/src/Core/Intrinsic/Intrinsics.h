#pragma once
#include <emmintrin.h>

union SSE_128 
{ 
	__m128 sse;
	float floats[4];
};