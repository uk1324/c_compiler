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
	Scanner* scanner;
	// Could rename this to previousToken
	Token current;
	Token previous;

	bool hadError;
	bool isSynchronizing;
} Parser;

// Add check that eof is reached
// Maybe change thye scanner this later
void ParserInit(Parser* parser, Scanner* scanner);
void ParserFree(Parser* parser);

StmtArray ParserParse(Parser* parser);