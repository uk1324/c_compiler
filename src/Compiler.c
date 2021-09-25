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

static int resetRegisterAllocationBitset(Compiler* compiler)
{
	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		compiler->isRegisterAllocated.array[i] = false;
	}

	compiler->isRegisterAllocated.rsp = true;
	compiler->isRegisterAllocated.rbp = true;
}

static int allocateRegister(Compiler* compiler)
{
	//for (int i = 0; i < REGISTER_COUNT; i++)
	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (compiler->isRegisterAllocated.array[i] == false)
		{
			compiler->isRegisterAllocated.array[i] = true;
			return i;
		}
	}

	return -1;
}

static void freeRegister(Compiler* compiler, int index)
{
	// Could assert here
	compiler->isRegisterAllocated.array[index] = false;
}

static const char* registerIndexToString(Register index, size_t registerSize)
{
	static const char* qwordRegisters[] = {
		"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
		"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
	};

	static const char* dwordRegisters[] = {
		"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
		"r8d",  "r9d",  "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
	};

	static const char* wordRegisters[] = {
		"ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
		"r8w",  "r9w",  "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
	};

	static const char* byteRegisters[] = {
		"al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
		"r8b",  "r9b",  "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
	};

	ASSERT(index < (sizeof(qwordRegisters) / sizeof(qwordRegisters[0])));

	switch (registerSize)
	{
		case 8: return qwordRegisters[index];
		case 4: return dwordRegisters[index];
		case 2: return wordRegisters[index];
		case 1: return byteRegisters[index];

		default:
			ASSERT_NOT_REACHED();
			break;
	}

	return NULL;
}

const char* dataSizeToString(size_t size)
{
	switch (size)
	{
		case 8: return "QWORD";
		case 4: return "DWORD";
		case 2: return "WORD";
		case 1: return "BYTE";

		default:
			ASSERT_NOT_REACHED();
			break;
	}

	return NULL;
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
		case RESULT_BASE_OFFSET:
			StringAppendFormat(&compiler->output, " %s [rbp-%u]", dataSizeToString(DataTypeSize(result.type)), result.location.baseOffset);
			break;

		case RESULT_REGISTER:
			StringAppendFormat(&compiler->output, " %s", registerIndexToString(result.location.reg, DataTypeSize(result.type)));
			break;

			// chnage later // wont work with bigger immediates
		case RESULT_IMMEDIATE:
			StringAppendFormat(&compiler->output, " %d", result.location.immediate);
			break;

		case RESULT_LABEL:
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
	result.locationType = RESULT_IMMEDIATE;
	// Maybe put a switch statement here later
	result.location.intImmediate = strtol(expr->literal.chars, NULL, 10);
	//DataType type;
	
	result.type.type = DATA_TYPE_INT;
	result.type.isUnsigned = false;
	return result;
}

// Returns the base offset to variable
static size_t allocateOnStack(Compiler* compiler, size_t size)
{
	// Fix this
	compiler->stackAllocationSize += ALIGN_UP_TO(4, size);
	return compiler->stackAllocationSize;
}

static bool isResultLocationMemory(ResultType locationType)
{
	return (locationType == RESULT_BASE_OFFSET) || (locationType == RESULT_LABEL);
}

static bool isResultLocationRegister(ResultType locationType)
{
	return locationType == RESULT_REGISTER;
}

static bool isResultLocationImmediate(ResultType locationType)
{
	return locationType == RESULT_IMMEDIATE;
}

// What to do if register allocation failure
// Could return a memory location but there are problems with stack alignment
static bool moveToRegister(Compiler* compiler, Result value, Result* result)
{
	Register reg = allocateRegister(compiler);

	if (reg == -1)
		return false;

	result->locationType = RESULT_REGISTER;
	result->location.reg = reg;

	emitInstruction(compiler, "mov %s,", registerIndexToString(result->location.reg, DataTypeSize(value.type)));
	emitResultLocation(compiler, value);

	return true;
}

Result allocateTemp(Compiler* compiler)
{
	Result temp;
	// Don't know the size of temp so just use QWORD
	temp.location.baseOffset = allocateOnStack(compiler, 8);
	temp.locationType = RESULT_BASE_OFFSET;
	temp.type.type = DATA_TYPE_LONG;
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
	if ((lhs.locationType == RESULT_IMMEDIATE) && (rhs.locationType == RESULT_IMMEDIATE))
	{
		result.locationType = RESULT_IMMEDIATE;
		result.location.immediate = rhs.location.immediate + lhs.location.immediate;
		return result;
	}

	Register savedRegisterLhs = -1;
	Result tempLocationLhs = {1, 1, 1};
	Register savedRegisterRhs = -1;
	Result tempLocationRhs = { 1, 1, 1 };

	if (lhs.locationType != RESULT_REGISTER)
	{
		if (moveToRegister(compiler, lhs, &lhs) == false)
		{
			savedRegisterLhs = 0;
			tempLocationLhs = allocateTemp(compiler);
			emitInstruction(compiler, "mov");
			emitResultLocation(compiler, tempLocationLhs);
			emitText(compiler, ", %s", registerIndexToString(savedRegisterLhs, 8));

			emitInstruction(compiler, "mov %s,", registerIndexToString(savedRegisterLhs, DataTypeSize(lhs.type)));
			emitResultLocation(compiler, lhs);
			
			lhs.locationType = RESULT_REGISTER;
			lhs.location.reg = savedRegisterLhs;
		}
	}
	if (rhs.locationType != RESULT_REGISTER)
	{
		if (moveToRegister(compiler, rhs, &rhs) == false)
		{
			for (int i = 0; i < REGISTER_COUNT; i++)
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
			emitText(compiler, ", %s", registerIndexToString(savedRegisterRhs, 8));

			emitInstruction(compiler, "mov %s,", registerIndexToString(savedRegisterRhs, DataTypeSize(rhs.type)));
			emitResultLocation(compiler, rhs);

			rhs.locationType = RESULT_REGISTER;
			rhs.location.reg = savedRegisterRhs;
		}
	}

	emitInstruction(compiler, "add");
	emitResultLocation(compiler, lhs);
	emitText(compiler, "%s", ",");
	emitResultLocation(compiler, rhs);
	if (rhs.locationType == RESULT_REGISTER && savedRegisterRhs == -1)
		freeRegister(compiler, rhs.location.reg);

	if (savedRegisterLhs != -1)
	{
		Result resultTemp = allocateTemp(compiler);
		emitInstruction(compiler, "mov", registerIndexToString(savedRegisterLhs, 8));
		emitResultLocation(compiler, resultTemp);
		emitText(compiler, ", %s", registerIndexToString(savedRegisterLhs, 8));

		emitInstruction(compiler, "mov %s,", registerIndexToString(savedRegisterLhs, 8));
		emitResultLocation(compiler, tempLocationLhs);
		lhs.locationType = RESULT_BASE_OFFSET;
	}

	if (savedRegisterRhs != -1)
	{
		emitInstruction(compiler, "mov %s,", registerIndexToString(savedRegisterRhs, 8));
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
	if (value.type.type == type.type)
		return value;

	ASSERT_NOT_REACHED();
}

static Result compileExprBinary(Compiler* compiler, ExprBinary* expr)
{
	Result a = compileExpr(compiler, expr->left);
	Result b = compileExpr(compiler, expr->right);
	DataType resultingType = binaryExpressionGetResultingType(a.type, b.type);
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
	result.locationType = RESULT_BASE_OFFSET;

	// Change this later to string view 
	StringView name = StringViewInit(expr->name.chars, expr->name.length);
	LocalVariable variable;
	if (LocalVariableTableGet(&compiler->locals, &name, &variable) == false)
	{
		errorAt(compiler, expr->name, "'%.*s' undeclared", expr->name.length, expr->name.chars);
	}

	result.location.baseOffset = variable.baseOffset;
	result.type = variable.type;
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
			break;
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
	variable.baseOffset = allocateOnStack(compiler, DataTypeSize(stmt->type));
	variable.type = stmt->type;
	StringView variableName = StringViewInit(stmt->name.chars, stmt->name.length);
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
	resetRegisterAllocationBitset(compiler);

	for (size_t i = 0; i < ast->size; i++)
	{
		compileStmt(compiler, ast->data[i]);
	}

	//printf("%d", compiler->stackAllocationSize);
	puts(compiler->output.chars);
}
