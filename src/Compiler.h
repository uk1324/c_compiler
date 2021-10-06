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

typedef struct
{
	size_t baseOffset;
	size_t size;
	bool isAllocated;
} Temp;

ARRAY_TEMPLATE_DECLARATION(TempArray, Temp)

typedef enum
{
	RESULT_LOCATION_BASE_OFFSET,
	RESULT_LOCATION_TEMP,
	RESULT_LOCATION_LABEL,
	RESULT_LOCATION_INT_CONSTANT,
} ResultLocationType;

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef uint8_t ubyte;
typedef uint16_t uword;
typedef uint32_t udword;
typedef uint64_t uqword;

// Later add system for allocating and freeing temps
// Maybe store the location in an array of temp objects
// It would store the size and location of temp
typedef struct
{
	ResultLocationType locationType;
	DataType dataType;

	union
	{
		// Used if RESULT_LOCATION_BASE_OFFSET
		uintptr_t baseOffset;

		// Used if RESUL_LOCATION_TEMP
		int tempIndex;

		// Used if RESULT_LOCATION_LABEL
		size_t labelIndex;

		// Used if RESULT_LOCATION_CONSTANT
		uint64_t constant;
		byte charConstant;
		word shortConstant;
		dword intConstant;
		qword longLongConstant;
	} location;
} Result;

typedef struct
{
	String textSection;
	String dataSection;

	bool hadError;

	// For line information
	const FileInfo* fileInfo;

	LocalVariableTable localVariables;

	size_t stackAllocationSize;
	TempArray temps;

	int labelCount;

} Compiler;

void CompilerInit(Compiler* compiler);
void CompilerFree(Compiler* compiler);
String CompilerCompile(Compiler* compiler, const FileInfo* fileInfo, const StmtArray* ast);