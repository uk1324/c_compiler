#include "Parser.h"
#include "TerminalColors.h" 
#include "Variable.h"

void ParserInit(Parser* parser)
{
	ScannerInit(&parser->scanner);
	//parser->scanner = scanner;
	//parser->current = ScannerNextToken(scanner);
}

void ParserFree(Parser* parser)
{
	ScannerFree(&parser->scanner);
}

static void advance(Parser* parser);
static bool isAtEnd(Parser* parser);

static void synchornize(Parser* parser)
{
	parser->isSynchronizing = false;
	while (isAtEnd(parser) == false)
	{
		advance(parser);
		switch (parser->current.type)
		{
			case TOKEN_INT:
			case TOKEN_SEMICOLON:
				return;
		}
	}
}

// Add fmt if needed later
static void errorAt(Parser* parser, Token* token, const char* message)
{
	if (parser->isSynchronizing)
		return;
	parser->hadError = true;
	parser->isSynchronizing = true;

	// The lines could not be fully scanned yet so this code is probaby garbage

	//size_t line = (size_t)token->line;
	//IntArray* lineStartOffsets = &parser->scanner.fileInfo->lineStartOffsets;
	//int lineLength = (line == lineStartOffsets->size)
	//	? parser->scanner.dataEnd - (parser->scanner.dataStart - lineStartOffsets->data[line - 1])
	//	: lineStartOffsets->data[line] - lineStartOffsets->data[line - 1];

	//int offsetInLine = (token->text.chars - parser->scanner.dataStart) - lineStartOffsets->data[line - 1];
	fprintf(stderr, "%s\n", message);
	//fprintf(
	//	stderr,
	//	"%s:%d:%d: " TERM_COL_RED "error: " TERM_COL_RESET "%s"
	//	"\n%.*s\n"
	//	"%*s" TERM_COL_GREEN "^%*s" TERM_COL_RESET "\n",
	//	parser->scanner.fileInfo->filename, line, offsetInLine, message,
	//	lineLength,
	//	&parser->scanner.dataStart[parser->scanner.fileInfo->lineStartOffsets.data[line - 1]],
	//	offsetInLine, " ",
	//	token->text.length - 1, "~"
	//);
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
		parser->current = ScannerNextToken(&parser->scanner);

		if (parser->current.type == TOKEN_ERROR)
			parser->hadError = true;
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

static void consume(Parser* parser, TokenType type, const char* errorMessage)
{
	if (match(parser, type) == false)
	{
		error(parser, errorMessage);
	}
}

static DataType dataType(Parser* parser)
{
	DataType type;

	// To make things like unsigned a; possible
	bool isSignednessSpecified = false;

	if (match(parser, TOKEN_UNSIGNED))
	{
		isSignednessSpecified = true;
		type.isUnsigned = true;
	}
	else
	{
		if (match(parser, TOKEN_SIGNED))
			isSignednessSpecified = true;
		type.isUnsigned = false;
	}

	if (match(parser, TOKEN_LONG))
	{
		if (match(parser, TOKEN_DOUBLE))
		{
			if (isSignednessSpecified)
			{
				error(parser, "cannot use a signedness specifier with long double");
				type.type = DATA_TYPE_ERROR;
				return type;
			}
			type.type = DATA_TYPE_LONG_DOUBLE;
		}

		if (match(parser, TOKEN_LONG))
		{
			match(parser, TOKEN_INT);
			type.type = DATA_TYPE_LONG_LONG;
		}

		type.type = DATA_TYPE_LONG;
	}
	else if (match(parser, TOKEN_SHORT))
	{
		match(parser, TOKEN_INT);
		type.type = DATA_TYPE_SHORT;
	}
	else if (match(parser, TOKEN_INT) || isSignednessSpecified)
	{
		type.type = DATA_TYPE_INT;
	}
	else if (match(parser, TOKEN_DOUBLE))
	{
		if (isSignednessSpecified)
		{
			error(parser, "cannot use a signedness specifier with double");
			type.type = DATA_TYPE_ERROR;
			return type;
		}
		type.type = DATA_TYPE_DOUBLE;
	}
	else if (match(parser, TOKEN_FLOAT))
	{
		if (isSignednessSpecified)
		{
			error(parser, "cannot use a signedness specifier with float");
			type.type = DATA_TYPE_ERROR;
			return type;
		}
		type.type = DATA_TYPE_FLOAT;
	}

	// Could return from some function sooner to make it less complex like moving the float function to the top
	// but the problem is more complex types like pointers but i might move that to another function.

	return type;
}

static Expr* expression(Parser* parser);

static Expr* intLiteral(Parser* parser)
{
	ExprNumberLiteral* expr = (ExprNumberLiteral*)ExprAllocate(sizeof(ExprNumberLiteral), EXPR_NUMBER_LITERAL);
	expr->literal = parser->previous;

	StringView token = parser->previous.text;
	const char* tokenStart = token.chars;
	const char* current = token.chars + token.length - 1;

	expr->type.isUnsigned = false;
	expr->type.type = DATA_TYPE_INT;

	if (*current == 'l')
	{
		current--;
		if ((current > tokenStart) && (*current == 'l'))
		{
			expr->type.type = DATA_TYPE_LONG_LONG;
		}
		else
		{
			expr->type.type = DATA_TYPE_LONG;
		}
		current--;
	}

	if (*current == 'L')
	{
		current--;
		if ((current > tokenStart) && (*current == 'L'))
		{
			expr->type.type = DATA_TYPE_LONG_LONG;
		}
		else
		{
			expr->type.type = DATA_TYPE_LONG;
		}
		current--;
	}

	if ((current > tokenStart) && ((*current == 'U') || (*current == 'u')))
	{
		expr->type.isUnsigned = true;
	}

	return (Expr*)expr;
}

static Expr* floatLiteral(Parser* parser)
{
	ExprNumberLiteral* expr = (ExprNumberLiteral*)ExprAllocate(sizeof(ExprNumberLiteral), EXPR_NUMBER_LITERAL);
	expr->literal = parser->previous;

	StringView token = parser->previous.text;
	const char* current = token.chars + token.length - 1;

	expr->type.type = DATA_TYPE_DOUBLE;

	if ((*current == 'l') || (*current == 'L'))
	{
		expr->type.type = DATA_TYPE_LONG_DOUBLE;
	}

	if ((*current == 'f') || (*current == 'F'))
	{
		expr->type.type = DATA_TYPE_FLOAT;
	}

	return (Expr*)expr;
}

static Expr* literal(Parser* parser)
{
	if (match(parser, TOKEN_INT_LITERAL))
	{
		return intLiteral(parser);
	}
	else if (match(parser, TOKEN_FLOAT_LITERAL))
	{
		return floatLiteral(parser);
	}
	else if (match(parser, TOKEN_IDENTIFIER))
	{
		ExprIdentifier* expr = (ExprIdentifier*)ExprAllocate(sizeof(ExprIdentifier), EXPR_IDENTIFIER);
		expr->name = parser->previous;
		return (Expr*)expr;
	}

	// Don't know if this is a good message
	error(parser, "Expected literal");
	return NULL;
}

static Expr* grouping(Parser* parser)
{
	if (match(parser, TOKEN_LEFT_PAREN))
	{
		ExprGrouping* expr = (ExprGrouping*)ExprAllocate(sizeof(ExprGrouping), EXPR_GROUPING);
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
		ExprUnary* expr = (ExprUnary*)ExprAllocate(sizeof(ExprUnary), EXPR_UNARY);
		expr->operator = parser->previous.type;
		expr->operand = expression(parser);
		return (Expr*)expr;
	}

	return grouping(parser);
}

// https://www.khanacademy.org/math/cc-sixth-grade-math/cc-6th-expressions-and-variables/cc-6th-evaluating-expressions/e/identifying-parts-of-expressions
static Expr* term(Parser* parser)
{
	Expr* expr = unary(parser);

	while ((match(parser, TOKEN_SLASH)) || (match(parser, TOKEN_ASTERISK)) || (match(parser, TOKEN_PERCENT)))
	{
		ExprBinary* temp = (ExprBinary*)ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		temp->left = expr;
		temp->operator = parser->previous;
		temp->right = unary(parser);
		expr = (Expr*)temp;
	}

	return expr;
}

static Expr* factor(Parser* parser)
{
	Expr* expr = term(parser);

	while ((match(parser, TOKEN_PLUS)) || (match(parser, TOKEN_MINUS)))
	{
		ExprBinary* temp = (ExprBinary*)ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		temp->left = expr;
		temp->operator = parser->previous;
		temp->right = term(parser);
		expr = (Expr*)temp;
	}
	
	return (Expr*)expr;
}

static Expr* expression(Parser* parser)
{
	return factor(parser);
}

//static StmtList* 

//#include "AstPrinter.h"

static Stmt* expressionStatement(Parser* parser)
{
	StmtExpression* expressionStmt = (StmtExpression*)StmtAllocate(sizeof(StmtExpression), STMT_EXPRESSION);
	expressionStmt->expresssion = expression(parser);
	//printExpr(expressionStmt->expresssion, 0);
	consume(parser, TOKEN_SEMICOLON, "expected ';'");
	return (Stmt*)expressionStmt;
}

// Later add support for multiple variables
// Don't know if it possible to compile that into multiple statment or just put all in one
static Stmt* variableDeclaration(Parser* parser)
{
	StmtVariableDeclaration* variableDeclaration = (StmtVariableDeclaration*)StmtAllocate(sizeof(StmtVariableDeclaration), STMT_VARIABLE_DECLARATION);
	variableDeclaration->type = dataType(parser);
	consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
	variableDeclaration->name = parser->previous;
	if (match(parser, TOKEN_EQUALS))
	{
		variableDeclaration->initializer = expression(parser);
	}
	else
	{
		variableDeclaration->initializer = NULL;
	}

	consume(parser, TOKEN_SEMICOLON, "expected ';'");

	return (Stmt*)variableDeclaration;
}

static Stmt* returnStatement(Parser* parser)
{
	StmtReturn* stmt = (StmtReturn*)StmtAllocate(sizeof(StmtReturn), STMT_RETURN);

	if (check(parser, TOKEN_SEMICOLON))
		stmt->returnValue = NULL;
	else
		stmt->returnValue = expression(parser);

	consume(parser, TOKEN_SEMICOLON, "Expected ';'");

	return (Stmt*)stmt;
}

bool isDataTypeStart(Parser* parser)
{
	return check(parser, TOKEN_INT)
		|| check(parser, TOKEN_SHORT)
		|| check(parser, TOKEN_LONG)
		|| check(parser, TOKEN_SIGNED)
		|| check(parser, TOKEN_UNSIGNED)
		|| check(parser, TOKEN_FLOAT)
		|| check(parser, TOKEN_DOUBLE);
}

static Stmt* statement(Parser* parser)
{
	if (isDataTypeStart(parser))
		return variableDeclaration(parser);
	else if (match(parser, TOKEN_RETURN))
		return returnStatement(parser);
	else
		return expressionStatement(parser);
}

StmtArray ParserParse(Parser* parser, const char* filename, StringView source, FileInfo* fileInfoToFillOut)
{
	parser->hadError = false;
	parser->isSynchronizing = false;


	fileInfoToFillOut->source = source;
	fileInfoToFillOut->filename = filename;
	ScannerReset(&parser->scanner, fileInfoToFillOut, source);
	//parser->scanner.fileInfo = fileInfoToFillOut;
	//parser->scanner.fileInfo->filename = filename;
	//parser->scanner.fileInfo->source = source;

	parser->current = ScannerNextToken(&parser->scanner);

	StmtArray array;
	StmtArrayInit(&array);
	while (isAtEnd(parser) == false)
	{
		StmtArrayAppend(&array, statement(parser));

		if (parser->isSynchronizing)
			synchornize(parser);
	}

	return array;
}