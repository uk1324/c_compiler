#include "Scanner.h"
#include "Ast.h"

#include <stdbool.h>

typedef struct
{
	Scanner* scanner;
	Token current;
	Token previous;

	bool hadError;

} Parser;

// Add check that eof is reached
// Maybe change thye scanner this later
void ParserInit(Parser* parser, Scanner* scanner);
void ParserFree(Parser* parser);

Expr* ParserParse(Parser* parser);