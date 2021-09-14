#include "Scanner.h"
#include "Ast.h"

#include <stdbool.h>

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