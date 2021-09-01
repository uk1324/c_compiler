#pragma once

#include "Ast.h"
#include "String.h"

#include <stdbool.h>
#include <stdint.h>

#define REGISTER_COUNT 16

typedef int Register;

typedef enum
{
	RESULT_REGISTER,
	RESULT_BASE_OFFSET,
	RESULT_LABEL,
	RESULT_IMMEDIATE
} ResultType;


typedef struct
{
	ResultType type;
	union
	{
		Register reg;
		uintptr_t baseOffset;
		int label;
		uint64_t immediate;
		// Don't know if I should use int here
		int intImmediate;
		double floatImmediate;
	};
} Result;

typedef struct
{
	String output;
	bool registerAllocationBitset[REGISTER_COUNT];
} Compiler;

void CompilerInit(Compiler* compiler);
void CompilerCompile(Compiler* compiler, Expr* ast);