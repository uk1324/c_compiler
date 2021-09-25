#pragma once

#include <stddef.h>

#define REGISTER_GP_COUNT 16
#define REGISTER_SIMD_COUNT 8

#define SIZE_QWORD 8
#define SIZE_DWORD 4
#define SIZE_WORD 2
#define SIZE_BYTE 1

// General purpose registers
typedef enum
{
	REGISTER_RAX,
	REGISTER_RBX,
	REGISTER_RCX,
	REGISTER_RDX,
	REGISTER_RSI,
	REGISTER_RDI,
	REGISTER_RBP,
	REGISTER_RSP,
	REGISTER_R8,
	REGISTER_R9,
	REGISTER_R10,
	REGISTER_R11,
	REGISTER_R12,
	REGISTER_R13,
	REGISTER_R14,
	REGISTER_R15,
	REGISTER_ERROR = -1
} RegisterGp;

typedef enum
{
	REGISTER_XMM0,
	REGISTER_XMM1,
	REGISTER_XMM2,
	REGISTER_XMM3,
	REGISTER_XMM4,
	REGISTER_XMM5,
	REGISTER_XMM6,
	REGISTER_XMM7
} RegisterSimd;

const char* RegisterGpToString(RegisterGp reg, size_t registerSize);
const char* RegisterSimdToString(RegisterSimd reg);