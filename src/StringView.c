#include "StringView.h"

StringView StringViewInit(const char* chars, size_t length)
{
	StringView view;
	view.chars = chars;
	view.length = length;
	return view;
}

size_t StringViewHash(const StringView* view)
{
	size_t hash = 0;
	for (char* chr = view->chars; chr < (view->chars + view->length); chr++)
		hash = *chr + 31 * hash;
	return hash;
}
