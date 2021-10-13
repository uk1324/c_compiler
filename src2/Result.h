#pragma once

#include "DataType.h"

#include <stdint.h>

typedef enum
{
	STORAGE_CLASS_NONE,
	STORAGE_CLASS_TYPEDEF,
	STORAGE_CLASS_EXTERN,
	STORAGE_CLASS_STATIC,
	STORAGE_CLASS_AUTO,
	STORAGE_CLASS_REGISTER,
} StorageClass;

typedef enum
{
	RESULT_LOCATION_BASE_OFFSET,
	RESULT_LOCATION_TEMP,
	RESULT_LOCATION_LABEL_COSTANT,
	RESULT_LOCATION_LABEL_INDEX,
	RESULT_LOCATION_LABEL_NAME,
	RESULT_LOCATION_INT_CONSTANT,
	RESULT_LOCATION_FLOAT_CONSTANT,
} ResultLocationType;

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef uint8_t ubyte;
typedef uint16_t uword;
typedef uint32_t udword;
typedef uint64_t uqword;

typedef struct
{
	StorageClass storageClass;

	DataType dataType;

	ResultLocationType locationType;
	union
	{
		// Used if RESULT_LOCATION_BASE_OFFSET
		uintptr_t baseOffset;

		// Used if RESUL_LOCATION_TEMP
		int tempIndex;

		// Used if RESULT_LOCATION_LABEL_INDEX and RESULT_LOCATION_LABEL_COSTANT
		size_t labelIndex;

		// Used if RESULT_LOCATION_INT_CONSTANT
		byte charConstant;
		word shortConstant;
		dword intConstant;
		qword longLongConstant;
		ubyte  unsignedCharConstant;
		uword  unsignedShortConstant;
		udword unsignedIntConstant;
		uqword unsignedLongLongConstant;

		// Used if RESULT_LOCATION_FLOAT_CONSTANT
		float floatConstant;
		double doubleConstant;
		long double longDoubleConstant;
	} location;
} Result;