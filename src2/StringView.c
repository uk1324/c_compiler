#include "StringView.h"

#include <string.h>

StringView StringViewInit(const char* chars, size_t length)
{
	StringView view;
	view.chars = chars;
	view.length = length;
	return view;
}

StringView StringViewFromString(const String* string)
{
	return StringViewInit(string->chars, string->length);
}

bool StringViewEquals(const StringView* a, const StringView* b)
{
	return (a->length == b->length) && (memcmp(a->chars, b->chars, a->length) == 0);
}

size_t StringViewHash(const StringView* view)
{
	size_t hash = 0;
	for (const char* chr = view->chars; chr < (view->chars + view->length); chr++)
		hash = *chr + 31 * hash;
	return hash;
}
