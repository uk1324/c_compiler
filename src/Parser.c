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
// Rename consume to expect
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

	fprintf(
		stderr, "%s:%d: " TERM_COL_RED "error: " TERM_COL_RESET "%s\n",
		parser->scanner.fileInfo->filename, token->line, message
	);
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
			return type;
		}

		if (match(parser, TOKEN_LONG))
		{
			match(parser, TOKEN_INT);
			type.type = DATA_TYPE_LONG_LONG;
			return type;
		}

		type.type = DATA_TYPE_LONG;
	}
	else if (match(parser, TOKEN_CHAR))
	{
		type.type = DATA_TYPE_CHAR;
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
		case TOKEN_CHAR_LITERAL:
			type.isUnsigned = false;
			type.type = DATA_TYPE_CHAR;
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

static Expr* comparison(Parser* parser)
{
	Expr* expr = factor(parser);

	while ((match(parser, TOKEN_LESS_THAN)) || (match(parser, TOKEN_LESS_THAN_EQUALS))
		|| (match(parser, TOKEN_MORE_THAN)) || (match(parser, TOKEN_MORE_THAN_EQUALS)))
	{
		ExprBinary* temp = (ExprBinary*)ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		temp->left = expr;
		temp->operator = parser->previous;
		temp->right = factor(parser);
		expr = (Expr*)temp;
	}

	return (Expr*)expr;
}

static Expr* equality(Parser* parser)
{
	Expr* expr = comparison(parser);

	while ((match(parser, TOKEN_BANG_EQUALS)) || (match(parser, TOKEN_EQUALS_EQUALS)))
	{
		ExprBinary* temp = (ExprBinary*)ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		temp->left = expr;
		temp->operator = parser->previous;
		temp->right = comparison(parser);
		expr = (Expr*)temp;
	}

	return (Expr*)expr;
}

static Expr* or(Parser* parser)
{
	Expr* expr = equality(parser);

	while ((match(parser, TOKEN_PIPE_PIPE)))
	{
		ExprBinary* temp = (ExprBinary*)ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		temp->left = expr;
		temp->operator = parser->previous;
		temp->right = equality(parser);
		expr = (Expr*)temp;
	}

	return (Expr*)expr;
}

static Expr* and(Parser* parser)
{
	Expr* expr = or(parser);

	while (match(parser, TOKEN_AMPERSAND_AMPERSAND))
	{
		ExprBinary* temp = (ExprBinary*)ExprAllocate(sizeof(ExprBinary), EXPR_BINARY);
		temp->left = expr;
		temp->operator = parser->previous;
		temp->right = or(parser);
		expr = (Expr*)temp;
	}

	return (Expr*)expr;
}

static Expr* assignment(Parser* parser)
{
	Expr* expr = and(parser);
		
	if (match(parser, TOKEN_EQUALS))
	{
		ExprAssignment* temp = EXPR_ALLOCATE(ExprAssignment, EXPR_ASSIGNMENT);
		temp->left = expr;
		temp->operator = parser->previous;
		// Shouldn't this be assignment ?
		temp->right = and(parser);
		expr = (Expr*)temp;
	}

	return expr;
}

static Expr* expression(Parser* parser)
{
	return assignment(parser);
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
	variableDeclaration->dataType = dataType(parser);
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

static Stmt* block(Parser* parser)
{
	StmtBlock* stmt = STMT_ALLOCATE(StmtBlock, STMT_BLOCK);
	StmtArrayInit(&stmt->satements);
	while ((isAtEnd(parser) == false) && (check(parser, TOKEN_RIGHT_BRACE) == false))
	{
		StmtArrayAppend(&stmt->satements, statement(parser));
	}
	consume(parser, TOKEN_RIGHT_BRACE, "Expected '}'");

	return (Stmt*)stmt;
}

static Stmt* ifStmt(Parser* parser)
{
	StmtIf* stmt = STMT_ALLOCATE(StmtIf, STMT_IF);
	consume(parser, TOKEN_LEFT_PAREN, "exptected '('");
	stmt->condition = expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "exptected ')'");
	stmt->thenBlock = statement(parser);
	if (match(parser, TOKEN_ELSE))
	{
		stmt->elseBlock = statement(parser);
	}
	else
	{
		stmt->elseBlock = NULL;
	}
	return (Stmt*)stmt;
}

static Stmt* whileStmt(Parser* parser)
{
	StmtWhileLoop* stmt = STMT_ALLOCATE(StmtWhileLoop, STMT_WHILE_LOOP);
	consume(parser, TOKEN_LEFT_PAREN, "exptected '('");
	stmt->condition = expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "exptected ')'");
	stmt->body = statement(parser);
	return (Stmt*)stmt;
}

static Stmt* forStmt(Parser* parser)
{
	StmtBlock* scope = STMT_ALLOCATE(StmtBlock, STMT_BLOCK);
	StmtArrayInit(&scope->satements);

	consume(parser, TOKEN_LEFT_PAREN, "exptected '('");

	if (match(parser, TOKEN_SEMICOLON) == false)
	{
		StmtArrayAppend(&scope->satements, statement(parser));
	}

	StmtWhileLoop* loop = STMT_ALLOCATE(StmtWhileLoop, STMT_WHILE_LOOP);

	if (match(parser, TOKEN_SEMICOLON))
	{
		ExprNumberLiteral* expr = EXPR_ALLOCATE(ExprNumberLiteral, EXPR_NUMBER_LITERAL);
		expr->dataType.type = DATA_TYPE_INT;
		expr->literal.line = parser->current.line;
		expr->literal.type = TOKEN_INT_LITERAL;
		expr->literal.text = StringViewInit("1", 1);
		loop->condition = (Expr*)expr;
	}
	else
	{
		loop->condition = expression(parser);
		consume(parser, TOKEN_SEMICOLON, "expected ';'");
	}

	Expr* iterationExpression = expression(parser);
	StmtExpression* expression = STMT_ALLOCATE(StmtExpression, STMT_EXPRESSION);
	expression->expresssion = iterationExpression;

	consume(parser, TOKEN_RIGHT_PAREN, "exptected ')'");

	StmtBlock* block = STMT_ALLOCATE(StmtBlock, STMT_BLOCK);
	StmtArrayInit(&block->satements);
	StmtArrayAppend(&block->satements, statement(parser));
	StmtArrayAppend(&block->satements, (Stmt*)expression);
	loop->body = (Stmt*)block;

	StmtArrayAppend(&scope->satements, (Stmt*)loop);

	return (Stmt*)scope;
}

static Stmt* breakStmt(Parser* parser)
{
	StmtBreak* stmt = STMT_ALLOCATE(StmtBreak, STMT_BREAK);
	stmt->token = parser->previous;
	consume(parser, TOKEN_SEMICOLON, "Expected ';'");
	return (Stmt*)stmt;
}

static Stmt* continueStmt(Parser* parser)
{
	StmtContinue* stmt = STMT_ALLOCATE(StmtContinue, STMT_CONTINUE);
	stmt->token = parser->previous;
	consume(parser, TOKEN_SEMICOLON, "Expected ';'");
	return (Stmt*)stmt;
}

static Stmt* putcharStmt(Parser* parser)
{
	StmtPutchar* stmt = STMT_ALLOCATE(StmtPutchar, STMT_PUTCHAR);
	consume(parser, TOKEN_LEFT_PAREN, "Expected '('");
	stmt->expresssion = expression(parser);
	consume(parser, TOKEN_RIGHT_PAREN, "Expected ')'");
	consume(parser, TOKEN_SEMICOLON, "Expected ';'");
	return (Stmt*)stmt;
}

bool isDataTypeStart(Parser* parser)
{
	return check(parser, TOKEN_INT)
		|| check(parser, TOKEN_CHAR)
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
	else if (match(parser, TOKEN_LEFT_BRACE))
		return block(parser);
	else if (match(parser, TOKEN_IF))
		return ifStmt(parser);
	else if (match(parser, TOKEN_WHILE))
		return whileStmt(parser);
	else if (match(parser, TOKEN_FOR))
		return forStmt(parser);
	else if (match(parser, TOKEN_BREAK))
		return breakStmt(parser);
	else if (match(parser, TOKEN_CONTINUE))
		return continueStmt(parser);
	else if (match(parser, TOKEN_PUTCHAR))
		return putcharStmt(parser);
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