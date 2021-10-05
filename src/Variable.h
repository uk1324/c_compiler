#pragma once

#include "String.h"
#include "Table.h"
#include "StringView.h"

#include <stdbool.h>
#include <stddef.h>

// Chars are unsigned by default

typedef enum
{
	DATA_TYPE_CHAR,
	DATA_TYPE_SHORT,
	DATA_TYPE_INT,
	DATA_TYPE_LONG,
	DATA_TYPE_LONG_LONG,

	DATA_TYPE_LONG_DOUBLE,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_FLOAT,

	DATA_TYPE_ERROR
} DataTypeType;

typedef struct
{
	DataTypeType type;

	bool isUnsigned;
	//union
	//{
	//	StructType* struct;
	//} as;
} DataType;

size_t DataTypeSize(DataType type);
bool DataTypeIsFloat(const DataType* type);
bool DataTypeIsInt(const DataType* type);

typedef struct
{
	DataType type;
	// Real position is [rbp-baseOffset]
	size_t baseOffset;
} LocalVariable;

typedef struct
{
	DataType type;
	int labelIndex;
} GlobalVariable;

TABLE_TEMPLATE_DECLARATION(LocalVariableTable, StringView, LocalVariable)