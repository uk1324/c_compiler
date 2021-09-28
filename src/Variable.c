#include "Variable.h"
#include "Assert.h"
#include "Generic.h"

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

size_t DataTypeSize(DataType type)
{
	// Add if for struct
	switch (type.type)
	{
		case DATA_TYPE_LONG: return 8;
		case DATA_TYPE_INT: return 4;
		case DATA_TYPE_SHORT: return 2;

	    default:
		    ASSERT_NOT_REACHED();
		    break;
	}
    return 0;
}
