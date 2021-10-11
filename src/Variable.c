#include "Variable.h"
#include "Assert.h"
#include "Generic.h"
// Probably should move the size information from Registers.h
#include "Registers.h"

static void copyStringView(StringView* dst, const StringView* src)
{
	*dst = *src;
}

static bool compareStringView(const StringView* a, const StringView* b)
{
	return (a->length == b->length) && (memcmp(a->chars, b->chars, a->length) == 0);
}

static void copyLocalVariable(LocalVariable* dst, const LocalVariable* src)
{
	*dst = *src;
}

static bool isStringViewNull(StringView* view)
{
	return view->chars == NULL;
}

static void setStringViewNull(StringView* view)
{
	view->chars = NULL;
}

TABLE_TEMPLATE_DEFINITION(LocalVariableTable, StringView, LocalVariable, StringViewHash, copyStringView, compareStringView, NO_OP_FUNCTION, copyLocalVariable, NO_OP_FUNCTION, isStringViewNull, setStringViewNull)

size_t DataTypeSize(const DataType* type)
{
	
	// Add if for struct
	switch (type->type)
	{
		case DATA_TYPE_CHAR:	   return SIZE_BYTE;
		case DATA_TYPE_SHORT:	   return SIZE_WORD;
		case DATA_TYPE_INT:		   return SIZE_DWORD;
		case DATA_TYPE_LONG:	   return SIZE_DWORD;
		case DATA_TYPE_LONG_LONG:  return SIZE_QWORD;
		case DATA_TYPE_FLOAT:      return SIZE_DWORD;
		case DATA_TYPE_DOUBLE:     return SIZE_QWORD;
		//case DATA_TYPE_LONG_DOUBLE: return

	    default:
		    ASSERT_NOT_REACHED();
		    break;
	}
    return 0;
}

bool DataTypeIsFloat(const DataType* type)
{
	return (type->type == DATA_TYPE_FLOAT)
		|| (type->type == DATA_TYPE_DOUBLE)
		|| (type->type == DATA_TYPE_LONG_DOUBLE);
}

bool DataTypeIsInt(const DataType* type)
{
	return (type->type == DATA_TYPE_CHAR)
		|| (type->type == DATA_TYPE_SHORT)
		|| (type->type == DATA_TYPE_INT)
		|| (type->type == DATA_TYPE_LONG)
		|| (type->type == DATA_TYPE_LONG_LONG);
}