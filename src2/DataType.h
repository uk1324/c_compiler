#pragma once

#include "Array.h"

#include <stdbool.h>

#define SIZE_QWORD 8
#define SIZE_DWORD 4
#define SIZE_WORD 2
#define SIZE_BYTE 1

typedef enum
{
	// Simple
	DATA_TYPE_CHAR,
	DATA_TYPE_SHORT,
	DATA_TYPE_INT,
	DATA_TYPE_LONG,
	DATA_TYPE_LONG_LONG,
	DATA_TYPE_FLOAT,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_LONG_DOUBLE,
	// Complex
	DATA_TYPE_POINTER,
	DATA_TYPE_ARRAY,
	DATA_TYPE_UNION,
	DATA_TYPE_STRUCT,
	DATA_TYPE_VOID,
	// Special
	DATA_TYPE_NONE,  // Used to check if a type matches during parsing 
	DATA_TYPE_ERROR // Used if an expression has invalid typse or when resolving a undefined type during parsing
} DataTypeType;

typedef enum
{
	TYPE_QUALIFIER_CONST,
	TYPE_QUALIFIER_VOLATILE,
	TYPE_QUALIFIER_NONE // Used for parsing
} TypeQualifier;

typedef struct DataType;

typedef struct
{
	int a;
} Struct;

typedef struct
{
	int b;
} Union;

typedef struct
{
	const struct DataType* type;
	int levelOfIndiretion;
} Pointer;

typedef struct
{
	const struct DataType* type;
} Array;

typedef struct
{
	DataTypeType type;

	// Not used if type is complex
	bool isUnsigned;

	bool isConst;
	bool isVolatile;

	union
	{
		Struct strct;
		Union unin;
		Pointer ptr;
		Array array;
	} as;

} DataType;

size_t DataTypeSize(const DataType* dataType);

ARRAY_TEMPLATE_DECLARATION(DataTypeArray, DataType)
