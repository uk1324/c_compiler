#include "Parser.h"
#include "TerminalColors.h" 
#include "Variable.h"
#include "Assert.h"

static void advance(Parser* parser);
static bool isAtEnd(Parser* parser);
static void errorAt(Parser* parser, Token* token, const char* message);
static void error(Parser* parser, const char* message);
static Token peek(Parser* parser);
static bool check(Parser* parser, TokenType type);
static bool match(Parser* parser, TokenType type);
static void consume(Parser* parser, TokenType type, const char* errorMessage);

static Stmt* expressionStatement(Parser* parser);
static Stmt* variableDeclaration(Parser* parser);
static Stmt* returnStatement(Parser* parser);

bool isDataTypeStart(Parser* parser);
static Stmt* statement(Parser* parser);

static DataType dataType(Parser* parser);
static Expr* literal(Parser* parser);
static Expr* grouping(Parser* parser);
static Expr* unary(Parser* parser);
static Expr* term(Parser* parser);
static Expr* factor(Parser* parser);
static Expr* expression(Parser* parser);

void ParserInit(Parser* parser)
{
	ScannerInit(&parser->scanner);
}

void ParserFree(Parser* parser)
{
	ScannerFree(&parser->scanner);
}

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

static void errorAt(Parser* parser, Token* token, const char* message)
{
	if (parser->isSynchronizing)
		return;
	parser->hadError = true;
	parser->isSynchronizing = true;

	fprintf(stderr, "%s\n", message);
}

static void error(Parser* parser, const char* message)
{
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

// Finding the data type in parsing is kindof pointless because the compiler has to switch between the types anyway.
// It does simplify things like char literals though.
static DataType tokenNumberLiteralToDataType(TokenType token)
{
	DataType type;

	switch (token)
	{
		case TOKEN_INT_LITERAL:
			type.isUnsigned = false;
			type.type = DATA_TYPE_INT;
			break;
		case TOKEN_UNSIGNED_INT_LITERAL:
			type.isUnsigned = true;
			type.type = DATA_TYPE_INT;
			break;
		case TOKEN_LONG_LITERAL:
			type.isUnsigned = false;
			type.type = DATA_TYPE_LONG;
			break;
		case TOKEN_UNSIGNED_LONG_LITERAL:
			type.isUnsigned = true;
			type.type = DATA_TYPE_LONG;
			break;
		case TOKEN_LONG_LONG_LITERAL:
			type.isUnsigned = false;
			type.type = DATA_TYPE_LONG_LONG;
			break;
		case TOKEN_UNSIGNED_LONG_LONG_LITERAL:
			type.isUnsigned = true;
			type.type = DATA_TYPE_LONG_LONG;
			break;
		case TOKEN_FLOAT_LITERAL:
			type.type = DATA_TYPE_FLOAT;
			break;
		case TOKEN_DOUBLE_LITERAL:
			type.type = DATA_TYPE_DOUBLE;
			break;
		case TOKEN_LONG_DOUBLE_LITERAL:
			type.type = DATA_TYPE_FLOAT;
			break;
		default:
			type.type = DATA_TYPE_ERROR;
	}

	return type;
}

static Expr* literal(Parser* parser)
{
	DataType dataType = tokenNumberLiteralToDataType(peek(parser).type);
	if (dataType.type != DATA_TYPE_ERROR)
	{
		advance(parser);
		ExprNumberLiteral* expr = EXPR_ALLOCATE(ExprNumberLiteral, EXPR_NUMBER_LITERAL);
		expr->dataType = dataType;
		expr->literal = parser->previous;
		return (Expr*)expr;
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

static Stmt* declaration(Parser* parser)
{

}

StmtArray ParserParse(Parser* parser, const char* filename, StringView source, FileInfo* fileInfoToFillOut)
{
	parser->hadError = false;
	parser->isSynchronizing = false;

	fileInfoToFillOut->source = source;
	fileInfoToFillOut->filename = filename;
	ScannerReset(&parser->scanner, fileInfoToFillOut);
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