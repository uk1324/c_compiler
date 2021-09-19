#define ALIGN_DOWN_TO(alignment, num) \
    (num - (num % alignment))

#define ALIGN_UP_TO(alignment, num) \
    (((num % alignment) == 0) ? num : (num + (alignment - (num % alignment))))

// Maybe define align up to call because [rsp + 8] has to be aligned to 16

#define CALL_STACK_ALIGNEMNT 16;