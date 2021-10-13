#include "String.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

String StringCopy(const char* string)
{
	String str;
	str.length = strlen(string);
	str.capacity = str.length + 1;
	str.chars = malloc(str.capacity);
	if (str.chars == NULL)
	{
		fputs("Failed to allocate string", stderr);
		exit(1);
	}
	memcpy(str.chars, string, str.length);
	str.chars[str.length] = '\0';
	return str;
}

String StringFromFile(const char* filename)
{
	String string;
	FILE* file = fopen(filename, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Cannot open file '%s'", filename);
		exit(1);
	}
	fseek(file, 0, SEEK_END);

	// This might return -1
	string.length = (size_t)ftell(file);
	string.capacity = string.length;
	fseek(file, 0, SEEK_SET);

	char* buffer = malloc(string.length + 1);
	if (buffer == NULL)
	{
		fputs("Failed to allocate memory", stderr);
		exit(1);
	}

	if (fread(buffer, sizeof(char), string.length, file) != string.length)
	{
		fputs("Failed to read file", stderr);
		exit(1);
	}

	buffer[string.length] = '\0';
	string.chars = buffer;
	fclose(file);
	return string;
}

String StringNonOwning(char* str)
{
	String string;
	string.chars = str;
	string.length = strlen(str);
	string.capacity = 0;
	return string;
}

static size_t growCapacity(size_t capacity)
{
	return capacity * 2;
}

void StringAppend(String* string, const char* str)
{
	StringAppendLen(string, str, strlen(str));
}

void StringAppendLen(String* string, const char* str, size_t length)
{
	if ((string->length + length + 1) > string->capacity)
	{
		size_t newCapacity = growCapacity(string->capacity) >= (string->length + length + 1)
			? growCapacity(string->capacity)
			: (string->length + length + 1);

		char* newChars = malloc(newCapacity);
		if (newChars == NULL)
		{
			fputs("Failed to reallocate string", stderr);
			exit(1);
		}
		memcpy(newChars, string->chars, string->length);
		free(string->chars);
		string->chars = newChars;
		string->capacity = newCapacity;
	}
	memcpy(string->chars + string->length, str, length);
	string->length = string->length + length;
	string->chars[string->length] = '\0';
}

void StringAppendVaFormat(String* string, const char* format, va_list arguments)
{
	va_list args;
	va_copy(args, arguments);

	int length = vsnprintf(NULL, 0, format, args);
	char* buffer = malloc(length + 1);
	if (buffer == NULL)
	{
		fputs("Failed to reallocate string", stderr);
		exit(1);
	}

	vsprintf(buffer, format, args);

	StringAppendLen(string, buffer, length);
	free(buffer);
}

void StringAppendFormat(String* string, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(string, format, args);
	va_end(args);
}

size_t StringHash(const String* string)
{
	size_t hash = 0;
	for (char* chr = string->chars; chr < (string->chars + string->length); chr++)
		hash = *chr + 31 * hash;
	return hash;
}

void StringFree(String* string)
{
	free(string->chars);
}