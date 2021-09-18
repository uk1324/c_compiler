#pragma once

#include "Array.h"
#include "Table.h"
#include "StringView.h"

typedef double JsonNumber;
typedef StringView JsonString;
typedef bool JsonBool;
typedef void* JsonNull;

struct JsonTable;
struct JsonArray;

typedef enum
{
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOL,
	JSON_NULL,
	JSON_ARRAY,
	JSON_OBJECT
} JsonType;

typedef struct
{
	JsonType type;
	union
	{
		JsonTable table;
		JsonArray array;
		JsonNumber number;
		JsonString string;
		JsonBool boolean;
	} value;
} JsonValue;

TABLE_TEMPLATE_DECLARATION(JsonTable, JsonString, JsonValue)
ARRAY_TEMPLATE_DECLARATION(JsonArray, JsonValue)

void JsonValueFree(JsonValue* value);