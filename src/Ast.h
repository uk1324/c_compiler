#pragma once

#include "Scanner.h"
#include "Variable.h"

// Should make more things const

typedef enum
{
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_INT_LITERAL,
	EXPR_GROUPING,
	EXPR_IDENTIFIER
} ExprType;

typedef struct
{
	ExprType type;
} Expr;

Expr* ExprAllocate(size_t size, ExprType type);
void ExprFree(Expr* expression);

typedef struct
{
	Expr expr;
	// Maybe later change to token
	TokenType operator;
	Expr* left;
	Expr* right;
} ExprBinary;

typedef struct
{
	Expr expr;
	TokenType operator;
	Expr* operand;
} ExprUnary;

typedef struct
{
	Expr expr;
	Expr* expression;
} ExprGrouping;

typedef struct
{
	Expr expr;
	Token name;
} ExprIdentifier;

// Maybe later add array literal and string
typedef struct
{
	Expr expr;
	Token literal;
} ExprIntLiteral;

typedef enum
{
	STMT_EXPRESSION,
	STMT_VARIABLE_DECLARATION
} StmtType;

typedef struct
{
	StmtType type;
} Stmt;

ARRAY_TEMPLATE_DECLARATION(StmtArray, Stmt*)

Stmt* StmtAllocate(size_t size, StmtType type);
void StmtFree(Stmt* statement);

typedef struct
{
	Stmt stmt;
	Expr* expresssion;
} StmtExpression;

typedef struct
{
	Stmt stmt;
	Token name;
	Expr* initializer; // Can be NULL
	// Later also add thing like is struct maybe
	DataType type;
} StmtVariableDeclaration;
