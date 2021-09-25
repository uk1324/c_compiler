#pragma once

#include "Ast.h"
#include "String.h"
#include "Variable.h"
#include "Parser.h"

#include <stdbool.h>
#include <stdint.h>

// https://wiki.osdev.org/Calling_Conventions
// Make Alignment.h with align macro
// Maybe store the variable size in variable type enum
// store additional data in variable if type is function or struct maybe function pointer but you should be able to call convert pointers so i don't know

// https://en.cppreference.com/w/c/language/bit_field
// Maybe use bitfield union for register allocation 

// https://en.wikipedia.org/wiki/C11_(C_standard_revision)
// Maybe use _Generic instead of align macro

// Annymous structs and unions are C11

//#define REGISTER_COUNT 16
#define REGISTER_COUNT 3

typedef int Register;

typedef enum
{
	RESULT_REGISTER,
	RESULT_BASE_OFFSET,
	RESULT_LABEL,
	RESULT_IMMEDIATE,
	// Find a better name
	RESULT_FLAGS,
} ResultType;


typedef struct
{
	// Find a better name
	ResultType locationType;
	DataType type;
	// Add isSigned or usigned
	// and Result size
	// result type can contain the size of the value but that might be confusing
	union
	{
		Register reg;
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
			bool array[REGISTER_COUNT];
			struct
			{
				bool rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp,
					 r8,  r9,  r10, r11, r12, r13, r14, r15;
			};
		};
	} isRegisterAllocated;

	// Later add scopes maybe store the current scope source so at the end of the scope the stack space is known
	LocalVariableTable locals;
	size_t stackAllocationSize;

} Compiler;

void CompilerInit(Compiler* compiler);
// Parser for line information
void CompilerCompile(Compiler* compiler, Parser* parser, StmtArray* ast);