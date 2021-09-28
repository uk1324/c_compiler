#pragma once

#include "Ast.h"
#include "String.h"
#include "Variable.h"
#include "Parser.h"
#include "Registers.h"

#include <stdbool.h>
#include <stdint.h>

// https://wiki.osdev.org/Calling_Conventions

// https://en.cppreference.com/w/c/language/bit_field
// Maybe use bitfield union for register allocation 

// https://en.wikipedia.org/wiki/C11_(C_standard_revision)
// Maybe use _Generic instead of align macro

typedef enum
{
	RESULT_LOCATION_REGISTER_GP,
	RESULT_LOCATION_REGISTER_SIMD,
	RESULT_LOCATION_BASE_OFFSET,
	RESULT_LOCATION_LABEL,
	RESULT_LOCATION_IMMEDIATE,
} ResultLocationType;


typedef struct
{
	ResultLocationType locationType;
	DataType dataType;

	union
	{
		RegisterGp registerGp;
		RegisterSimd registerSimd;
		uintptr_t baseOffset;
		int label;
		uint64_t immediate;
		// Don't know if I should use int here
		int intImmediate;
		double floatImmediate;
	} location;
} Result;

typedef struct
{
	String output;

	bool hadError;

	// For line information
	Parser* parser;
	// Spit this form here later
	struct
	{
		union
		{
			bool array[REGISTER_GP_COUNT];
			struct
			{
				bool rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp,
					 r8,  r9,  r10, r11, r12, r13, r14, r15;
			} registerGp;
		} as;
	} isRegisterGpAllocated;

	struct
	{
		union
		{
			bool array[REGISTER_SIMD_COUNT];
			struct
			{
				bool xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
			} registerGp;
		} as;
	} isRegisterSimdAllocated;

	// Later add scopes maybe store the current scope source so at the end of the scope the stack space is known
	LocalVariableTable locals;
	size_t stackAllocationSize;

} Compiler;

void CompilerInit(Compiler* compiler);
// Parser for line information
void CompilerCompile(Compiler* compiler, Parser* parser, StmtArray* ast);