#include "Compiler.h"
#include "Assert.h"

#include <stdarg.h>

static int resetRegisterAllocationBitset(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		compiler->registerAllocationBitset[i] = false;
	}
}

static int allocateRegister(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (compiler->registerAllocationBitset[i] == false)
		{
			return i;
		}
	}

	return -1;
}

static const char* registerIndexToString(Register index)
{
	const char* indexToRegister[] = {
		"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
		"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
	};

	ASSERT(index < (sizeof(indexToRegister) / sizeof(indexToRegister[0])));

	return indexToRegister[index];
}

static const char* emitInstruction(Compiler* compiler, const char* format, ...)
{
	StringAppend(&compiler->output, "\t");
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->output, format, args);
	va_end(args);
}

static Result compileExpr(Compiler* compiler, Expr* expr);

static Result compileExprIntLiteral(Compiler* compiler, ExprIntLiteral* expr)
{
	// Every instruction except mov uses a 32 bit immediate mov uses a 64 bit immediate
	Result result;
	result.type = RESULT_IMMEDIATE;
	// Maybe put a switch statement here later
	result.intImmediate = strtol(expr->literal.chars, NULL, 10);
	return result;
}

static Result compileAddition(Compiler* compiler, Result lhs, Result rhs)
{
	if ((lhs.type == RESULT_IMMEDIATE) && (rhs.type == RESULT_IMMEDIATE))
	{
		
	}
}

static Result compileExprBinary(Compiler* compiler, ExprBinary* expr)
{
	Result lhs = compileExpr(compiler, expr->left);
	Result rhs = compileExpr(compiler, expr->right);

	switch (expr->operator)
	{
		case TOKEN_PLUS:
			return compileAddition(compiler, lhs, rhs);

		default:
			ASSERT_NOT_REACHED();
			break;
	}

}

static Result compileExpr(Compiler* compiler, Expr* expr)
{
	switch (expr->type)
	{

		case EXPR_BINARY: return compileExprBinary(compiler, (ExprBinary*)expr); 
		case EXPR_INT_LITERAL: return compileExprIntLiteral(compiler, (ExprIntLiteral*)expr);

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

void CompilerInit(Compiler* compiler)
{
}

void CompilerCompile(Compiler* compiler, Expr* ast)
{
	CompilerInit(compiler);
	compiler->output = StringCopy("");

	compileExpr(compiler, ast);

	puts(compiler->output.chars);
}
