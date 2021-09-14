#pragma once

#include "String.h"
#include "Table.h"
#include "StringView.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
	VARIABLE_INT,
	VARIABLE_SHORT,
} VariableType;

size_t VariableTypeSize(VariableType type);

typedef struct
{
	VariableType type;

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
	VariableType type;
	// Real offset is bp[-baseOffset]
	size_t baseOffset;
} LocalVariable;

typedef struct
{
	VariableType type;
	int labelIndex;
} GlobalVariable;

//TABLE_TEMPLATE_DECLARATION(VariableTable, String, Variable)
TABLE_TEMPLATE_DECLARATION(LocalVariableTable, StringView, LocalVariable)