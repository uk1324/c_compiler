#include "IntArray.h"
#include "Generic.h"

static void copyInt(int* destination, int* source)
{
	*destination = *source;
}

ARRAY_TEMPLATE_DEFINITION(IntArray, int, copyInt, NO_OP_FUNCTION)