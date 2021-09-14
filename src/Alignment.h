#define ALIGN_DOWN_TO(x, num) \
    (num - num % x)

#define ALIGN_UP_TO(x, num) \
    (num + (x - (num % x)))

// Maybe define align up to call because [rsp + 8] has to be aligned to 16

#define CALL_STACK_ALIGNEMNT 16;