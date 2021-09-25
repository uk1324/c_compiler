#pragma once

#include "String.h"
#include "Table.h"
#include "StringView.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
	DATA_TYPE_INT,
	DATA_TYPE_SHORT,
	DATA_TYPE_LONG,
	DATA_TYPE_LONG_DOUBLE,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_FLOAT
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

typedef struct
{
	DataType type;

	size_t baseOffset;
	
	bool isUnsigned;
	//union 
	//{
	//	// Struct
	//	// function
	//};

} Variable;

typedef struct
{
	DataType type;
	// Real offset is bp[-baseOffset]
	size_t baseOffset;
} LocalVariable;

typedef struct
{
	DataType type;
	int labelIndex;
} GlobalVariable;

//TABLE_TEMPLATE_DECLARATION(VariableTable, String, Variable)
TABLE_TEMPLATE_DECLARATION(LocalVariableTable, StringView, LocalVariable)