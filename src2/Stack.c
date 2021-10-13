#include "Compiler.h"
#include "Stack.h"

size_t allocateSingleVariableOnStack(Compiler* compiler, size_t size)
{
	// On x86 data in memory should be aligned to the size of the data.
	// If memory is not aligned the cpu might need to perform 2 loads
	// or with some instructions the program might crash.
	compiler->stackFrameSize = ALIGN_UP_TO(size, compiler->stackFrameSize) + size;
	return compiler->stackFrameSize;
}

size_t allocateArrayVariableOnStack(Compiler* compiler, size_t size, size_t count)
{
	compiler->stackFrameSize = ALIGN_UP_TO(size, compiler->stackFrameSize) + size * count;
}