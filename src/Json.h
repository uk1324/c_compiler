//#pragma once
//
//#include "Array.h"
//#include "Table.h"
//#include "String.h"
//
//typedef double JsonNumber;
//typedef String JsonString;
//
//struct JsonTable;
//struct JsonArray;
//
//typedef enum
//{
//	JSON_ARRAY,
//	JSON_NUMBER,
//	JSON_BOOL,
//	JSON_OBJECT,
//	JSON_NULL,
//	JSON_STRING
//} JsonType;
//
//typedef struct Json
//{
//	JsonType type;
//	union
//	{
//		JsonTable table;
//		JsonArray array;
//		JsonNumber number;
//	} value;
//} Json;
//
//TABLE_TEMPLATE_DECLARATION(JsonTable, char*, Json)
//ARRAY_TEMPLATE_DECLARATION(JsonArray, Json)