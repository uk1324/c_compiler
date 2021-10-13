#include "DataType.h"
#include "Generic.h"
#include "Assert.h"

size_t DataTypeSize(const DataType* dataType)
{
	switch (dataType->type)
	{
		case DATA_TYPE_CHAR: return SIZE_BYTE;
		case DATA_TYPE_SHORT: return SIZE_WORD;
		case DATA_TYPE_INT: return SIZE_DWORD;
		case DATA_TYPE_LONG: return SIZE_DWORD;
		case DATA_TYPE_LONG_LONG: return SIZE_QWORD;
		case DATA_TYPE_FLOAT: return SIZE_DWORD;
		case DATA_TYPE_DOUBLE: return SIZE_QWORD;

		case DATA_TYPE_POINTER: return SIZE_QWORD;

		default:
			ASSERT_NOT_REACHED();
			return 0;
	}
}

static void copyDataType(DataType* dst, const DataType* src)
{
	*dst = *src;
}

ARRAY_TEMPLATE_DEFINITION(DataTypeArray, DataType, copyDataType, NO_OP_FUNCTION)