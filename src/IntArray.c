#include "IntArray.h"

static void copyInt(int* destination, int* source)
{
	*destination = *source;
}

ARRAY_TEMPLATE_DEFINITION(IntArray, int, copyInt, (void(*)))