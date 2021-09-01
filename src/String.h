#pragma once

// Disable fopen_s warnings
#define _CRT_SECURE_NO_WARNINGS

#include <stddef.h>

typedef struct
{
	char* chars;
	size_t length;
	size_t capacity;
} String;

//void StringInit()

String StringCopy(const char* string);
String StringFromFile(const char* filename);
void StringAppendVaFormat(String* string, const char* format, va_list arguments);
void StringAppendFormat(String* string, const char* format, ...);
// Maybe make a non owning string in append so both string and constchar* can be appended
void StringAppend(String* string, const char* str);
void StringAppendLen(String* string, const char* str, size_t length);
void StringFree(String* string);