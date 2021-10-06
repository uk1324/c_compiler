#pragma once

#include "Scanner.h"
#include "Ast.h"

#include <stdbool.h>

// Maybe add a parse time data type
// It will contain types like identifier, struct, struct identifier, and fucntion pointer
// struct type could be used for declarations like
// struct { int x, int y } a;

typedef struct
{
	Scanner scanner;
	Token current;
	Token previous;

	bool hadError;
	bool isSynchronizing;
} Parser;

void ParserInit(Parser* parser);
void ParserFree(Parser* parser);
StmtArray ParserParse(Parser* parser, const char* filename, StringView source, FileInfo* fileInfoToFillOut);