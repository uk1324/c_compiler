#include "Compiler.h"
#include "Assert.h"
#include "Alignment.h"

#include <stdarg.h>

void CompilerInit(Compiler* compiler)
{
	LocalVariableTableInit(&compiler->locals);
}

void CompilerFree(Compiler* compiler)
{
	LocalVariableTableFree(&compiler->locals);
}

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
	ASSERT(reg != REGISTER_RSP);
	ASSERT(reg != REGISTER_RBP);
	ASSERT(reg != REGISTER_GP_SCRATCH);
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

	compiler->isRegisterGpAllocated.as.array[REGISTER_GP_SCRATCH] = true;
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

void freeTemp(Compiler* compiler, const Result* temp)
{

}

// Handles operations that have encoding op reg, reg
static Result compileSimpleIntBinaryExpression(Compiler* compiler, Result lhs, Result rhs, TokenType operator)
{
	Result result;

	RegisterGp savedRegisterLhs = REGISTER_ERROR;
	Result tempLocationLhs = { 0, 0, 0};

	if (lhs.locationType != RESULT_LOCATION_REGISTER_GP)
	{
		// Couldn't allocate register for lhs
		if (moveToRegister(compiler, lhs, &lhs) == false)
		{
			// Take any register to move the value into
			savedRegisterLhs = REGISTER_RAX;

			// Move the value to a temp memory location
			// Could later try moving it to the scratch register but I don't know if the rhs would need to use it
			// The temp would still be needed for stornig the final result
			tempLocationLhs = allocateTemp(compiler);
			emitInstruction(compiler, "mov");
			emitResultLocation(compiler, tempLocationLhs);
			emitText(compiler, ", %s", RegisterGpToString(savedRegisterLhs, 8));

			// Move lhs to register.
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
			emitInstruction(compiler, "mov %s,", RegisterGpToString(REGISTER_GP_SCRATCH, DataTypeSize(rhs.dataType)));
			emitResultLocation(compiler, rhs);

			rhs.locationType = RESULT_LOCATION_REGISTER_GP;
			rhs.location.registerGp = REGISTER_GP_SCRATCH;
		}
	}

	const char* op = "";

	switch (operator)
	{
		case TOKEN_PLUS:       op = "add"; break;
		case TOKEN_MINUS:      op = "sub"; break;
		case TOKEN_AMPERSAND:  op = "and"; break;
		case TOKEN_PIPE:       op = "or";  break;
		case TOKEN_CIRCUMFLEX: op = "xor"; break;
			
		default:
			ASSERT_NOT_REACHED();
			break;
	}

	emitInstruction(compiler, op);
	emitResultLocation(compiler, lhs);
	emitText(compiler, ",");
	emitResultLocation(compiler, rhs);

	if ((rhs.locationType == RESULT_LOCATION_REGISTER_GP) && (rhs.location.registerGp != REGISTER_GP_SCRATCH))
	{
		freeGpRegister(compiler, rhs.location.registerGp);
	}

	if (savedRegisterLhs != REGISTER_ERROR)
	{

		// Move the saved register to the sratch register
		emitInstruction(compiler, "mov %s,", RegisterGpToString(REGISTER_GP_SCRATCH, SIZE_QWORD));
		emitResultLocation(compiler, tempLocationLhs);

		// Move lhs to the temp location
		emitInstruction(compiler, "mov");
		emitResultLocation(compiler, tempLocationLhs);
		emitText(compiler, ", %s", RegisterGpToString(savedRegisterLhs, DataTypeSize(tempLocationLhs.dataType)));

		// Move the previous value of the register back to it
		emitInstruction(
			compiler, "mov %s, %s",
			RegisterGpToString(savedRegisterLhs, SIZE_QWORD),
			RegisterGpToString(REGISTER_GP_SCRATCH, SIZE_QWORD)
		);
		
		// Add location type temp the size of the temp might be larger that the data stored in it
		lhs.locationType = RESULT_LOCATION_BASE_OFFSET;
		lhs.location.baseOffset = tempLocationLhs.location.baseOffset;
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

	//result.type = DATA_TYPE_ERROR;

	return result;
}

static Result convertToType(Compiler* compiler, DataType type, Result value)
{
	if (value.dataType.type == type.type)
		return value;

	ASSERT_NOT_REACHED();
}

static Result evaluateConstantBinaryExpression(Compiler* compiler, const Result* lhs, const Result* rhs, Token operator)
{
	Result result;
	result.dataType = lhs->dataType;
	result.locationType = RESULT_LOCATION_IMMEDIATE;
	
	switch (rhs->dataType.type)
	{
		case DATA_TYPE_INT: 
		{
			switch (operator.type)
			{
			case TOKEN_PLUS: result.location.intImmediate = lhs->location.intImmediate + rhs->location.intImmediate; break;
			case TOKEN_MINUS: result.location.intImmediate = lhs->location.intImmediate - rhs->location.intImmediate; break;

			default:
				errorAt(compiler, operator, "Invalid operands for binary operator %s", "later add type");
				break;
			}
		}

	}

	return result;
}

static Result compileExprBinary(Compiler* compiler, const ExprBinary* expr)
{
	Result a = compileExpr(compiler, expr->left);
	Result b = compileExpr(compiler, expr->right);

	DataType resultingType = binaryExpressionGetResultingType(a.dataType, b.dataType);
	if (resultingType.type == DATA_TYPE_ERROR)
	{
		// Don't know if anything else could cause conversion to fail
		errorAt(compiler, expr->operator, "Invalid operator %.*s", expr->operator.text.length, expr->operator.text.chars);
	}

	Result lhs = convertToType(compiler, resultingType, a);
	Result rhs = convertToType(compiler, resultingType, b);

	if ((lhs.locationType == RESULT_LOCATION_IMMEDIATE) && (rhs.locationType == RESULT_LOCATION_IMMEDIATE))
	{
		return evaluateConstantBinaryExpression(compiler, &lhs, &rhs, expr->operator);
	}

	switch (expr->operator.type)
	{
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_AMPERSAND:
		case TOKEN_PIPE:
		case TOKEN_CIRCUMFLEX:
			return compileSimpleIntBinaryExpression(compiler, lhs, rhs, expr->operator.type);

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

static void compileStmtExpression(Compiler* compiler, const StmtExpression* stmt)
{
	freeGpRegisters(compiler);
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

static void compileStmtReturn(Compiler* compiler, const StmtReturn* stmt)
{
	if (stmt->returnValue != NULL)
	{
		Result returnValue = compileExpr(compiler, stmt->returnValue);

		if ((returnValue.locationType == RESULT_LOCATION_REGISTER_GP)
	     && (returnValue.location.registerGp != REGISTER_RAX))
		{
			emitInstruction(compiler, "mov %s,", RegisterGpToString(REGISTER_RAX, DataTypeSize(returnValue.dataType)));
			emitResultLocation(compiler, returnValue);
		}
	}
	//emitInstruction(compiler, "ret");
}

static void compileStmt(Compiler* compiler, const Stmt* stmt)
{
	switch (stmt->type)
	{
		case STMT_EXPRESSION: compileStmtExpression(compiler, (StmtExpression*)stmt); break;
		case STMT_VARIABLE_DECLARATION: compileStmtVariableDeclaration(compiler, (StmtVariableDeclaration*)stmt); break;
		case STMT_RETURN: compileStmtReturn(compiler, (StmtReturn*)stmt); break;

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

// Make it return a String object;
String CompilerCompile(Compiler* compiler, const FileInfo* fileInfo, const StmtArray* ast)
{
	compiler->fileInfo = fileInfo;
	compiler->output = StringCopy("global _start\n_start:\n\tmov rbp, rsp");
	compiler->stackAllocationSize = 0;
	freeGpRegisters(compiler);

	for (size_t i = 0; i < ast->size; i++)
	{
		compileStmt(compiler, ast->data[i]);
	}

	emitInstruction(compiler, "mov rdi, rax");
	emitInstruction(compiler, "mov rax, 60");
	emitInstruction(compiler, "syscall");

	return compiler->output;
}
