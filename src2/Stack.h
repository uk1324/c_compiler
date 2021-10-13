#pragma once

typedef struct Compiler;

#define ALIGN_DOWN_TO(alignment, num) \
    (num - (num % alignment))

#define ALIGN_UP_TO(alignment, num) \
    (((num % alignment) == 0) ? num : (num + (alignment - (num % alignment))))


#define CALL_STACK_ALIGNEMNT 16;

// The System V ABI specfies that the [rsp + 8] has to be aligned to 16 bytes at the time of call so SIMD mov instruction will work.
// The call instruction pushes the rbp onto the stack so when the function starts executing the stack will be aligned to 16 bytes.
#define ALIGN_STACK_FOR_CALL(num) \
    ALIGN_UP_TO(CALL_STACK_ALIGNEMNT, num + 8)

size_t allocateSingleVariableOnStack(Compiler* compiler, size_t size);
size_t allocateArrayVariableOnStack(Compiler* compiler, size_t size, size_t count);