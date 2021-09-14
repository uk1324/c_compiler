#include "Compiler.h"
#include "Assert.h"
#include "Alignment.h"

#include <stdarg.h>

static void errorAt(Compiler* compiler, Token location, const char* format, ...)
{
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

static int resetRegisterAllocationBitset(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		compiler->isRegisterAllocated.array[i] = false;
	}
}

static int allocateRegister(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (compiler->isRegisterAllocated.array[i] == false)
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
	result.value.intImmediate = strtol(expr->literal.chars, NULL, 10);
	return result;
}

// Returns the base offset to variable
static size_t allocateOnStack(Compiler* compiler, size_t size)
{
	// Fix this
	compiler->stackAllocationSize += ALIGN_UP_TO(4, size);
	return compiler->stackAllocationSize;
}

static Result compileAddition(Compiler* compiler, Result lhs, Result rhs)
{
	Result result;
	// Later do conversion Type binaryExpressionConversionType
	if ((lhs.type == RESULT_IMMEDIATE) && (rhs.type == RESULT_IMMEDIATE))
	{
		result.type = RESULT_IMMEDIATE;
		result.value.immediate = rhs.value.immediate + lhs.value.immediate;
	}
	// Later add support for global maybe function isResultMemory
	else if ((lhs.type == RESULT_BASE_OFFSET) && (rhs.type == RESULT_BASE_OFFSET))
	{
		Register reg = allocateRegister(compiler);
		if (reg == -1)
		{
			//emitInstruction()
		}
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

static Result compileExprIdentifier(Compiler* compiler, ExprIdentifier* expr)
{
	Result result;
	result.type = RESULT_BASE_OFFSET;

	// Change this later to string view 
	StringView name = StringViewInit(expr->name.chars, expr->name.length);
	LocalVariable variable;
	if (LocalVariableTableGet(&compiler->locals, &name, &variable) == false)
	{
		//error();
	}

	result.value.baseOffset = variable.baseOffset;
	return result;
}

static Result compileExpr(Compiler* compiler, Expr* expr)
{
	switch (expr->type)
	{

		case EXPR_BINARY: return compileExprBinary(compiler, (ExprBinary*)expr); 
		case EXPR_INT_LITERAL: return compileExprIntLiteral(compiler, (ExprIntLiteral*)expr);
		case EXPR_IDENTIFIER: return compileExprIdentifier(compiler, (ExprIdentifier*)expr);

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

static void compileStmtExpression(Compiler* compiler, StmtExpression* stmt)
{
	
}

static void compileStmtVariableDeclaration(Compiler* compiler, StmtVariableDeclaration stmt)
{
	Result result;
	result.type = RESULT_BASE_OFFSET;
	result.value.baseOffset = allocateOnStack(compiler, VariableTypeSize(stmt.type));
	return result;
}

static void compileStmt(Compiler* compiler, Stmt* stmt)
{
	switch (stmt->type)
	{
		case STMT_EXPRESSION: compileStmtExpression(compiler, (StmtExpression*)stmt); break;
		case STMT_VARIABLE_DECLARATION: compileStmtExpression(compiler, (StmtVariableDeclaration*)stmt); break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

void CompilerInit(Compiler* compiler)
{

}

void CompilerCompile(Compiler* compiler, Parser* parser, StmtArray* ast)
{
	CompilerInit(compiler);
	compiler->parser = parser;
	compiler->output = StringCopy("");
	compiler->stackAllocationSize = 0;

	for (size_t i = 0; i < ast->size; i++)
	{
		compileStmt(compiler, ast->data[i]);
	}

	printf("%d", compiler->stackAllocationSize);
	puts(compiler->output.chars);
}
