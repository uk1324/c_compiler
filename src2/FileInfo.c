#include "FileInfo.h"
#include "Assert.h"
#include "Generic.h"

FileInfo FileInfoInit(FileInfo* fileInfo)
{
	SizetArrayInit(&fileInfo->lineStartOffsets);
	return *fileInfo;
}

void FileInfoFree(FileInfo* fileInfo)
{
	SizetArrayFree(&fileInfo->lineStartOffsets);
}

StringView FileInfoGetLine(const FileInfo* fileInfo, size_t lineNumber)
{
	ASSERT(lineNumber < fileInfo->lineStartOffsets.size);

}

static void copySizet(size_t* dst, const size_t* src)
{
	*dst = *src;
}

ARRAY_TEMPLATE_DEFINITION(SizetArray, size_t, copySizet, NO_OP_FUNCTION)