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
	printf("\n");
}


static void freeGpRegister(Compiler* compiler, RegisterGp reg)
{
	ASSERT((size_t)reg < REGISTER_GP_COUNT);
	compiler->isRegisterGpAllocated.as.array[reg] = false;
}

static void freeGpRegisters(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_GP_COUNT; i++)
	{
		compiler->isRegisterGpAllocated.as.array[i] = false;
	}

	compiler->isRegisterGpAllocated.as.registerGp.rsp = true;
	compiler->isRegisterGpAllocated.as.registerGp.rbp = true;
}

static RegisterGp allocateGpRegister(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_GP_COUNT; i++)
	{
		if (compiler->isRegisterGpAllocated.as.array[i] == false)
		{
			compiler->isRegisterGpAllocated.as.array[i] = true;
			return i;
		}
	}

	return REGISTER_ERROR;
}

const char* dataSizeToString(size_t size)
{
	switch (size)
	{
		case SIZE_QWORD: return "QWORD";
		case SIZE_DWORD: return "DWORD";
		case SIZE_WORD:  return "WORD";
		case SIZE_BYTE:  return "BYTE";

		default:
			ASSERT_NOT_REACHED();
			return NULL;
	}
}

static void emitInstruction(Compiler* compiler, const char* format, ...)
{
	StringAppend(&compiler->output, "\n\t");
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->output, format, args);
	va_end(args);
}

static void emitText(Compiler* compiler, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->output, format, args);
	va_end(args);
}

static void emitResultLocation(Compiler* compiler, Result result)
{
	switch (result.locationType)
	{
		case RESULT_LOCATION_BASE_OFFSET:
			StringAppendFormat(&compiler->output, " %s [rbp-%u]", dataSizeToString(DataTypeSize(result.dataType)), result.location.baseOffset);
			break;

		case RESULT_LOCATION_REGISTER_GP:
			StringAppendFormat(&compiler->output, " %s", RegisterGpToString(result.location.registerGp, DataTypeSize(result.dataType)));
			break;

			// chnage later // wont work with bigger immediates
		case RESULT_LOCATION_IMMEDIATE:
			StringAppendFormat(&compiler->output, " %d", result.location.immediate);
			break;

		case RESULT_LOCATION_LABEL:
			StringAppendFormat(&compiler->output, " .L%d", result.location.label);
			break;

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

static Result compileExpr(Compiler* compiler, Expr* expr);

static Result compileExprIntLiteral(Compiler* compiler, ExprIntLiteral* expr)
{
	// Every instruction except mov uses a 32 bit immediate mov uses a 64 bit immediate
	Result result;
	result.locationType = RESULT_LOCATION_IMMEDIATE;
	// Maybe put a switch statement here later
	result.location.intImmediate = strtol(expr->literal.text.chars, NULL, 10);
	//DataType type;
	
	result.dataType.type = DATA_TYPE_INT;
	result.dataType.isUnsigned = false;
	return result;
}

// Returns the base offset
static size_t allocateSingleVariableOnStack(Compiler* compiler, size_t size)
{
	// On x86 data in memory should be aligned to the size of the data.
	// If memory is not aligned the cpu might need to perform 2 loads or with some instructions the program might crash.
	compiler->stackAllocationSize = ALIGN_UP_TO(size, compiler->stackAllocationSize) + size;
	return compiler->stackAllocationSize;
}

static bool isResultLocationMemory(ResultLocationType locationType)
{
	return (locationType == RESULT_LOCATION_BASE_OFFSET) || (locationType == RESULT_LOCATION_LABEL);
}

static bool isResultLocationRegister(ResultLocationType locationType)
{
	return locationType == RESULT_LOCATION_REGISTER_GP;
}

static bool isResultLocationImmediate(ResultLocationType locationType)
{
	return locationType == RESULT_LOCATION_IMMEDIATE;
}

// What to do if register allocation failure
// Could return a memory location but there are problems with stack alignment
static bool moveToRegister(Compiler* compiler, Result value, Result* result)
{
	RegisterGp reg = allocateGpRegister(compiler);

	if (reg == -1)
		return false;

	result->locationType = RESULT_LOCATION_REGISTER_GP;
	result->location.registerGp = reg;

	emitInstruction(compiler, "mov %s,", RegisterGpToString(result->location.registerGp, DataTypeSize(value.dataType)));
	emitResultLocation(compiler, value);

	return true;
}

Result allocateTemp(Compiler* compiler)
{
	Result temp;
	// Don't know the size of temp so just use QWORD
	temp.location.baseOffset = allocateSingleVariableOnStack(compiler, 8);
	temp.locationType = RESULT_LOCATION_BASE_OFFSET;
	temp.dataType.type = DATA_TYPE_LONG;
	return temp;
}

// Requires types to already be converted
// Doesn't support volatile
// Doesn't handle register allocation failure
// Could just move both operands to registers so it is easier
// It is hard to find the best instruction because of things like result being in the flags
// Don't know how to handle that yet
// Add allocateTemporary if no temeporary is free allocate new on the stack
static Result compileAddition(Compiler* compiler, Result lhs, Result rhs)
{
	Result result;
	// Later do conversion Type binaryExpressionConversionType
	if ((lhs.locationType == RESULT_LOCATION_IMMEDIATE) && (rhs.locationType == RESULT_LOCATION_IMMEDIATE))
	{
		result.locationType = RESULT_LOCATION_IMMEDIATE;
		result.location.immediate = rhs.location.immediate + lhs.location.immediate;
		return result;
	}

	RegisterGp savedRegisterLhs = -1;
	Result tempLocationLhs = {1, 1, 1};
	RegisterGp savedRegisterRhs = -1;
	Result tempLocationRhs = { 1, 1, 1 };

	if (lhs.locationType != RESULT_LOCATION_REGISTER_GP)
	{
		if (moveToRegister(compiler, lhs, &lhs) == false)
		{
			savedRegisterLhs = 0;
			tempLocationLhs = allocateTemp(compiler);
			emitInstruction(compiler, "mov");
			emitResultLocation(compiler, tempLocationLhs);
			emitText(compiler, ", %s", RegisterGpToString(savedRegisterLhs, 8));

			emitInstruction(compiler, "mov %s,", RegisterGpToString(savedRegisterLhs, DataTypeSize(lhs.dataType)));
			emitResultLocation(compiler, lhs);
			
			lhs.locationType = RESULT_LOCATION_REGISTER_GP;
			lhs.location.registerGp = savedRegisterLhs;
		}
	}
	if (rhs.locationType != RESULT_LOCATION_REGISTER_GP)
	{
		if (moveToRegister(compiler, rhs, &rhs) == false)
		{
			for (int i = 0; i < REGISTER_GP_COUNT; i++)
			{
				if (i != savedRegisterLhs)
				{
					savedRegisterRhs = i;
					break;
				}
			}
			tempLocationRhs = allocateTemp(compiler);
			emitInstruction(compiler, "mov");
			emitResultLocation(compiler, tempLocationRhs);
			emitText(compiler, ", %s", RegisterGpToString(savedRegisterRhs, 8));

			emitInstruction(compiler, "mov %s,", RegisterGpToString(savedRegisterRhs, DataTypeSize(rhs.dataType)));
			emitResultLocation(compiler, rhs);

			rhs.locationType = RESULT_LOCATION_REGISTER_GP;
			rhs.location.registerGp = savedRegisterRhs;
		}
	}

	emitInstruction(compiler, "add");
	emitResultLocation(compiler, lhs);
	emitText(compiler, "%s", ",");
	emitResultLocation(compiler, rhs);
	if (rhs.locationType == RESULT_LOCATION_REGISTER_GP && savedRegisterRhs == -1)
		freeGpRegister(compiler, rhs.location.registerGp);

	if (savedRegisterLhs != -1)
	{
		Result resultTemp = allocateTemp(compiler);
		emitInstruction(compiler, "mov", RegisterGpToString(savedRegisterLhs, 8));
		emitResultLocation(compiler, resultTemp);
		emitText(compiler, ", %s", RegisterGpToString(savedRegisterLhs, 8));

		emitInstruction(compiler, "mov %s,", RegisterGpToString(savedRegisterLhs, 8));
		emitResultLocation(compiler, tempLocationLhs);
		lhs.locationType = RESULT_LOCATION_BASE_OFFSET;
	}

	if (savedRegisterRhs != -1)
	{
		emitInstruction(compiler, "mov %s,", RegisterGpToString(savedRegisterRhs, 8));
		emitResultLocation(compiler, tempLocationRhs);
	}

	return lhs;
}

static DataType binaryExpressionGetResultingType(DataType a, DataType b)
{
	DataType result;

	if ((a.type == DATA_TYPE_LONG_DOUBLE) || (b.type == DATA_TYPE_LONG_DOUBLE))
	{
		result.type = DATA_TYPE_LONG_DOUBLE;
		return result;
	}

	if ((a.type == DATA_TYPE_DOUBLE) || (b.type == DATA_TYPE_DOUBLE))
	{
		result.type = DATA_TYPE_DOUBLE;
		return result;
	}

	if ((a.type == DATA_TYPE_FLOAT) || (b.type == DATA_TYPE_FLOAT))
	{
		result.type = DATA_TYPE_FLOAT;
		return result;
	}

	// Change this later
	result.type = (DataTypeSize(a) > DataTypeSize(b)) ? a.type : b.type;
	return result;
}

static Result convertToType(Compiler* compiler, DataType type, Result value)
{
	if (value.dataType.type == type.type)
		return value;

	ASSERT_NOT_REACHED();
}

static Result compileExprBinary(Compiler* compiler, ExprBinary* expr)
{
	Result a = compileExpr(compiler, expr->left);
	Result b = compileExpr(compiler, expr->right);
	DataType resultingType = binaryExpressionGetResultingType(a.dataType, b.dataType);
	Result lhs = convertToType(compiler, resultingType, a);
	Result rhs = convertToType(compiler, resultingType, b);

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
	result.locationType = RESULT_LOCATION_BASE_OFFSET;

	// Change this later to string view 
	StringView name = StringViewInit(expr->name.text.chars, expr->name.text.length);
	LocalVariable variable;
	if (LocalVariableTableGet(&compiler->locals, &name, &variable) == false)
	{
		errorAt(compiler, expr->name, "'%.*s' undeclared", expr->name.text.length, expr->name.text.chars);
	}

	result.location.baseOffset = variable.baseOffset;
	result.dataType = variable.type;
	return result;
}

static Result compileExpr(Compiler* compiler, Expr* expr)
{
	switch (expr->type)
	{
		case EXPR_BINARY: return compileExprBinary(compiler, (ExprBinary*)expr); 
		case EXPR_INT_LITERAL: return compileExprIntLiteral(compiler, (ExprIntLiteral*)expr);
		case EXPR_IDENTIFIER: return compileExprIdentifier(compiler, (ExprIdentifier*)expr);
		case EXPR_GROUPING: return compileExpr(compiler, ((ExprGrouping*)expr)->expression);

		default:
			ASSERT_NOT_REACHED();
	}	
}

static void compileStmtExpression(Compiler* compiler, StmtExpression* stmt)
{
	compileExpr(compiler, stmt->expresssion);
}

static void compileStmtVariableDeclaration(Compiler* compiler, StmtVariableDeclaration* stmt)
{
	//Result initializer;

	//if (stmt->initializer != NULL)

	LocalVariable variable;
	variable.baseOffset = allocateSingleVariableOnStack(compiler, DataTypeSize(stmt->type));
	variable.type = stmt->type;
	StringView variableName = stmt->name.text;
	LocalVariableTableSet(&compiler->locals, &variableName, variable);
	
	// if is integral data type
	emitInstruction(compiler, "mov %s [rbp-%u],", dataSizeToString(DataTypeSize(variable.type)), variable.baseOffset);
	// Wont work with 2 memory operands
	emitResultLocation(compiler, compileExpr(compiler, stmt->initializer));

	//Result result;
	//result.type = RESULT_BASE_OFFSET;
	////return result;
}

static void compileStmt(Compiler* compiler, Stmt* stmt)
{
	switch (stmt->type)
	{
		case STMT_EXPRESSION: compileStmtExpression(compiler, (StmtExpression*)stmt); break;
		case STMT_VARIABLE_DECLARATION: compileStmtVariableDeclaration(compiler, (StmtVariableDeclaration*)stmt); break;
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

void CompilerInit(Compiler* compiler)
{
	LocalVariableTableInit(&compiler->locals);
}

void CompilerCompile(Compiler* compiler, Parser* parser, StmtArray* ast)
{
	CompilerInit(compiler);
	compiler->parser = parser;
	compiler->output = StringCopy("");
	compiler->stackAllocationSize = 0;
	freeGpRegisters(compiler);

	for (size_t i = 0; i < ast->size; i++)
	{
		compileStmt(compiler, ast->data[i]);
	}

	//printf("%d", compiler->stackAllocationSize);
	puts(compiler->output.chars);
}
