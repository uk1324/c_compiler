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
	EXPR_IDENTIFIER,
	EXPR_ASSIGNMENT
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

typedef struct
{
	Expr expr;
	Expr* left;
	Expr* right;
	Token operator;
} ExprAssignment;

typedef enum
{
	STMT_EXPRESSION,
	STMT_VARIABLE_DECLARATION,
	STMT_RETURN,
	STMT_BLOCK,
	STMT_IF,
	STMT_WHILE_LOOP,
	STMT_DO_WHILE_LOOP,
	STMT_BREAK,
	STMT_CONTINUE,
	STMT_PUTCHAR
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
	DataType dataType;
} StmtVariableDeclaration;

typedef struct
{
	Stmt stmt;
	Token location;
	Expr* returnValue; // Can be NULL
} StmtReturn;

typedef struct
{
	Stmt stmt;
	StmtArray satements;
} StmtBlock;

typedef struct
{
	Stmt stmt;
	Expr* condition;
	Stmt* thenBlock;
	Stmt* elseBlock;
} StmtIf;

typedef struct
{
	Stmt stmt;
	Expr* condition;
	Stmt* body;
} StmtWhileLoop;

typedef struct
{
	Stmt stmt;
	Token token;
} StmtBreak;

typedef struct
{
	Stmt stmt;
	Token token;
} StmtContinue;

typedef struct
{
	Stmt stmt;
	Expr* expresssion;
} StmtPutchar;