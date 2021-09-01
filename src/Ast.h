#pragma once

#include "Scanner.h"

typedef enum
{
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_INT_LITERAL,
	EXPR_GROUPING
} ExprType;

typedef struct
{
	ExprType type;
} Expr;

void* ExprAllocate(size_t size, ExprType type);
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

// Maybe later split to array literal and number 
typedef struct
{
	Expr expr;
	Token literal;
} ExprIntLiteral;