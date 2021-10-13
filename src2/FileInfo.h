#pragma once

#include "Array.h"
#include "StringView.h"

ARRAY_TEMPLATE_DECLARATION(SizetArray, size_t)

typedef struct
{
	const char* filename;
	StringView source;
	// Line numbers are counted from 0.
	SizetArray lineStartOffsets;
} FileInfo;

FileInfo FileInfoInit(FileInfo* fileInfo);
void FileInfoFree(FileInfo* fileInfo);
StringView FileInfoGetLine(const FileInfo* fileInfo, size_t lineNumber);