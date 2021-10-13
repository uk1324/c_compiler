#pragma once

#include "String.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
	const char* chars;
	size_t length;
} StringView;

StringView StringViewInit(const char* chars, size_t length);
StringView StringViewFromString(const String* string);
bool StringViewEquals(const StringView* a, const StringView* b);
size_t StringViewHash(const StringView* view);