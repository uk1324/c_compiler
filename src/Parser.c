#include "Parser.h"
#include "TerminalColors.h" 

void ParserInit(Parser* parser, Scanner* scanner)
{
	parser->scanner = scanner;
	parser->current = ScannerNextToken(scanner);
	parser->hadError = false;
}

void ParserFree(Parser* parser)
{

}

// Add fmt if needed later

static void errorAt(Parser* parser, Token* token, const char* message)
{
	size_t line = (size_t)token->line;
	IntArray* lineStartOffsets = &parser->scanner->lineStartOffsets;

	int lineLength;

	if (line == lineStartOffsets->size)
	{
		lineLength = parser->scanner->dataEnd - (parser->scanner->dataStart - lineStartOffsets->data[line - 1]);
	}
	else
	{
		lineLength = lineStartOffsets->data[line] - lineStartOffsets->data[line - 1];
	}

	//int lineLength = (line == lineStartOffsets->size)
	//	? parser->scanner->dataEnd - lineStartOffsets->data[line - 1]
	//	: lineStartOffsets->data[line - 1] - lineStartOffsets->data[line - 2];

	int offsetInLine = (token->chars - parser->scanner->dataStart) - lineStartOffsets->data[line - 1];

	printf("type: %d\n", token->type);

	fprintf(
		stderr,
		"%s:%d:%d: " TERM_COL_RED "error: " TERM_COL_RESET "%s"
		"\n%.*s\n"
		"%*s" TERM_COL_GREEN "^%*s" TERM_COL_RESET "\n",
		parser->scanner->filename, line, offsetInLine, message,
		lineLength,
		&parser->scanner->dataStart[parser->scanner->lineStartOffsets.data[line - 1]],
		offsetInLine, " ",
		token->length - 1, "~"
	);
}

static void error(Parser* parser, const char* message)
{
	// Later add support for scanner errors
	errorAt(parser, &parser->current, message);
}

static bool isAtEnd(Parser* parser)
{
	return parser->current.type == TOKEN_EOF;
}

static void advance(Parser* parser)
{
	if (isAtEnd(parser) == false)
	{
		parser->previous = parser->current;
		parser->current = ScannerNextToken(parser->scanner);
		printf("token_t: %d\n", parser->current.type);
	}
}

static Token peek(Parser* parser)
{
	return parser->current;
}

static bool check(Parser* parser, TokenType type)
{
	return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type)
{
	if (check(parser, type))
	{
		advance(parser);
		return true;
	}
	return false;
}

static void consume(Parser* parser, TokenType type, const char* message)
{
	if (match(parser, type) == false)
	{
		error(parser, message);
	}
}

static Expr* expression(Parser* parser);

static Expr* literal(Parser* parser)
{
	if (match(parser, TOKEN_NUMBER))
	{
		ExprIntLiteral* expr = ExprAllocate(sizeof(ExprIntLiteral), EXPR_INT_LITERAL);
		expr->literal = parser->previous;
		
		return (Expr*)expr;
	}
	else
	{
		// Don't know if this is a good message
		error(parser, "Expected literal");
	}
}

static Expr* grouping(Parser* parser)
{
	if (match(parser, TOKEN_LEFT_PAREN))
	{
		ExprGrouping* expr = ExprAllocate(sizeof(ExprGrouping), EXPR_GROUPING);
		expr->expression = expression(parser);
		consume(parser, TOKEN_RIGHT_PAREN, "expected ')' after expression");
		return (Expr*)expr;
	}

	return literal(parser);
}

static Expr* unary(Parser* parser)
{
	if ((match(parser, TOKEN_PLUS)) || (match(parser, TOKEN_MINUS)))
	{
		ExprUnary* expr = ExprAllocate(sizeof(ExprUnary), EXPR_UNARY);
		expr->operator = parser->previous.type;
		expr->operand = expression(parser);
		return (Expr*)expr;
	}

	return grouping(parser);
}

// https://www.khanacademy.org/math/cc-sixth-grade-math/cc-6th-expressions-and-variables/cc-6th-evaluating-expressions/e/identifying-parts-of-expressions
static Expr* term(Parser* parser)
{
	Expr* left = unary(parser);

	if ((match(parser, TOKEN_SLASH)) || (match(parser, TOKEN_ASTERISK)) || (match(parser, TOKEN_PERCENT)))
	{
		ExprBinary* expr = ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		expr->left = left;
		expr->operator = parser->previous.type;
		expr->right = term(parser);
		return (Expr*)expr;
	}

	return left;
}

static Expr* factor(Parser* parser)
{
	Expr* left = term(parser);

	if ((match(parser, TOKEN_PLUS)) || (match(parser, TOKEN_MINUS)))
	{
		ExprBinary* expr = ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		expr->left = left;
		expr->operator = parser->previous.type;
		expr->right = factor(parser);
		return (Expr*)expr;
	}

	return left;
}

static Expr* expression(Parser* parser)
{
	return factor(parser);
}

Expr* ParserParse(Parser* parser)
{
	return expression(parser);
}
