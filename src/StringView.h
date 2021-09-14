#pragma once

#include <stddef.h>

typedef struct
{
	// Maybe make it const size_t and const char* const
	const char* chars;
	size_t length;
} StringView;

StringView StringViewInit(const char* chars, size_t length);
size_t StringViewHash(const StringView* view);