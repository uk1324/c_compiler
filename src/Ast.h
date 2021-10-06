#pragma once

#include "Scanner.h"
#include "Variable.h"

// Should make more things const

// Could implement the visitor pattern with a cost of one virtual call
// There would be a vtable in the Expr or Stmt class contaning a function for every visitor
// for example Result acceptCompilerVisitor(Compiler* compiler, Stmt* stmt);
// In this example the compiler would implement all the methods like CompilerAcceptExprBinary
// That the vtable function would use.
// Could automate generating of both using macros.

typedef enum
{
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_NUMBER_LITERAL,
	EXPR_GROUPING,
	EXPR_IDENTIFIER
} ExprType;

typedef struct
{
	ExprType type;
} Expr;

Expr* ExprAllocate(size_t size, ExprType type);
#define EXPR_ALLOCATE(dataType, exprType) ((dataType*)StmtAllocate(sizeof(dataType), exprType))
void ExprFree(Expr* expression);

typedef struct
{
	Expr expr;
	// Maybe later change to token
	Token operator;
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

// Chars have type int

// Maybe later add array literal and string
typedef struct
{
	Expr expr;
	Token literal;
	DataType dataType;
} ExprNumberLiteral;

typedef enum
{
	STMT_EXPRESSION,
	STMT_VARIABLE_DECLARATION,
	STMT_RETURN
} StmtType;

typedef struct
{
	StmtType type;
} Stmt;

ARRAY_TEMPLATE_DECLARATION(StmtArray, Stmt*)

Stmt* StmtAllocate(size_t size, StmtType type);
#define STMT_ALLOCATE(dataType, stmtType) ((dataType*)StmtAllocate(sizeof(dataType), stmtType))
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

typedef struct
{
	Stmt stmt;
	Token location;
	Expr* returnValue; // Can be NULL
} StmtReturn;
