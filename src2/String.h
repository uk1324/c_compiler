#pragma once

#include <stddef.h>

typedef struct
{
	char* chars;
	size_t length;
	size_t capacity;
} String;

String StringCopy(const char* string);
String StringFromFile(const char* filename);
String StringNonOwning(char* str);
void StringAppendVaFormat(String* string, const char* format, va_list arguments);
void StringAppendFormat(String* string, const char* format, ...);
size_t StringHash(const String* string);

void StringAppend(String* string, const char* str);
void StringAppendLen(String* string, const char* str, size_t length);
void StringFree(String* string);