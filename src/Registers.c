#include "Registers.h"
#include "Assert.h"

const char* RegisterGpToString(RegisterGp reg, size_t registerSize)
{
	static const char* qwordRegisters[] = {
		[REGISTER_RAX] = "rax", [REGISTER_RBX] = "rbx", [REGISTER_RCX] = "rcx", [REGISTER_RDX] = "rdx",
		[REGISTER_RSI] = "rsi", [REGISTER_RDI] = "rdi", [REGISTER_RBP] = "rbp", [REGISTER_RSP] = "rsp",
		[REGISTER_R8]  = "r8",  [REGISTER_R9]  = "r9",  [REGISTER_R10] = "r10", [REGISTER_R11] = "r11",
		[REGISTER_R12] = "r12", [REGISTER_R13] = "r13", [REGISTER_R14] = "r14", [REGISTER_R15] = "r15",
	};

	static const char* dwordRegisters[] = {
		[REGISTER_RAX] = "eax",  [REGISTER_RBX] = "ebx",  [REGISTER_RCX] = "ecx",  [REGISTER_RDX]  = "edx",
		[REGISTER_RSI] = "esi",  [REGISTER_RDI] = "edi",  [REGISTER_RBP] = "ebp",  [REGISTER_RSP]  = "esp",
		[REGISTER_R8]  = "r8d",  [REGISTER_R9]  = "r9d",  [REGISTER_R10] = "r10d", [REGISTER_R11]  = "r11d",
		[REGISTER_R12] = "r12d", [REGISTER_R13] = "r13d", [REGISTER_R14] = "r14d", [REGISTER_R15]  = "r15d",
	};
	
	static const char* wordRegisters[] = {
		[REGISTER_RAX] = "ax",   [REGISTER_RBX] = "bx",   [REGISTER_RCX] = "cx",   [REGISTER_RDX]  = "dx",
		[REGISTER_RSI] = "si",   [REGISTER_RDI] = "di",   [REGISTER_RBP] = "bp",   [REGISTER_RSP]  = "sp",
		[REGISTER_R8]  = "r8w",  [REGISTER_R9]  = "r9w",  [REGISTER_R10] = "r10w", [REGISTER_R11]  = "r11w",
		[REGISTER_R12] = "r12w", [REGISTER_R13] = "r13w", [REGISTER_R14] = "r14w", [REGISTER_R15]  = "r15w",
	};

	static const char* byteRegisters[] = {
		[REGISTER_RAX] = "al",   [REGISTER_RBX] = "bl",   [REGISTER_RCX] = "cl",   [REGISTER_RDX]  = "dl",
		[REGISTER_RSI] = "sil",  [REGISTER_RDI] = "dil",  [REGISTER_RBP] = "bpl",  [REGISTER_RSP]  = "spl",
		[REGISTER_R8]  = "r8b",  [REGISTER_R9]  = "r9b",  [REGISTER_R10] = "r10b", [REGISTER_R11]  = "r11b",
		[REGISTER_R12] = "r12b", [REGISTER_R13] = "r13b", [REGISTER_R14] = "r14b", [REGISTER_R15]  = "r15b",
	};

	ASSERT((size_t)reg < (sizeof(qwordRegisters) / sizeof(qwordRegisters[0])));

	switch (registerSize)
	{
		case SIZE_QWORD: return qwordRegisters[reg];
		case SIZE_DWORD: return dwordRegisters[reg];
		case SIZE_WORD:  return wordRegisters[reg];
		case SIZE_BYTE:  return byteRegisters[reg];

		default:
			ASSERT_NOT_REACHED();
			return NULL;
	}
}

const char* RegisterSimdToString(RegisterSimd reg)
{
	static const char* simdRegisters[] = {
		[REGISTER_XMM0] = "xmm0", [REGISTER_XMM1] = "xmm1",	[REGISTER_XMM2] = "xmm2", [REGISTER_XMM3] = "xmm3",
		[REGISTER_XMM4] = "xmm4", [REGISTER_XMM5] = "xmm5", [REGISTER_XMM6] = "xmm6", [REGISTER_XMM7] = "xmm7"
	};

	ASSERT((size_t)reg < (sizeof(simdRegisters) / sizeof(simdRegisters[0])));

	return simdRegisters[reg];
}
