#include "Compiler.h"
#include "Assert.h"
#include "Alignment.h"
#include "Generic.h"
#include "TerminalColors.h"

#include <stdarg.h>

static void errorAt(Compiler* compiler, Token token, const char* message, ...);

static void emitCode(Compiler* compiler, const char* format, ...);
static void emitInstruction(Compiler* compiler, const char* format, ...);
static void emitData(Compiler* compiler, const char* format, ...);

static void emitResult(Compiler* compiler, const Result* result);
static void emitIntConstant(Compiler* compiler, const Result* result);
static void emitMovToRegisterGp(Compiler* compiler, RegisterGp reg, const Result* result);
static void emitMovFromRegisterGp(Compiler* compiler, const Result* result, RegisterGp reg);
static void emitMovToRegisterSimd(Compiler* compiler, RegisterSimd reg, const Result* result);
static void emitMovFromRegisterSimd(Compiler* compiler, const Result* result, RegisterSimd reg);

static int allocateLabel(Compiler* compiler);
static size_t allocateSingleVariableOnStack(Compiler* compiler, size_t size);
static Result allocateTemp(Compiler* compiler, const DataType* dataType);
static void freeTemp(Compiler* compiler, const Result* temp);

static int getIntegerConversionRank(const DataType* dataType);
static DataType binaryExpressionGetResultingType(const DataType* a, const DataType* b);

static Result convertToType(Compiler* compiler, const Result* value, const DataType* type);

static Result compileExpr(Compiler* compiler, const Expr* expr);
static Result compileExprNumberLiteral(Compiler* compiler, const ExprNumberLiteral* expr);
// Maybe use this later for asignment operators like +=
static Result compileExprBinary(Compiler* compiler, const ExprBinary* expr);
static Result compileSimpleIntExprBinary(Compiler* compiler, const char* op, const Result* lhs, const Result* rhs);

static void compileStmt(Compiler* compiler, const Stmt* stmt);
static void compileStmtExpression(Compiler* compiler, const StmtExpression* stmt);
static void compileStmtReturn(Compiler* compiler, const StmtReturn* stmt);

void CompilerInit(Compiler* compiler)
{
	LocalVariableTableInit(&compiler->localVariables);
	TempArrayInit(&compiler->temps);
}

void CompilerFree(Compiler* compiler)
{
	LocalVariableTableFree(&compiler->localVariables);
	TempArrayFree(&compiler->temps);
}

void errorAt(Compiler* compiler, Token token, const char* message, ...)
{
	compiler->hadError = true;

	const FileInfo* fileInfo = compiler->fileInfo;
	size_t lineOffset = token.text.chars - (fileInfo->source.chars + fileInfo->lineStartOffsets.data[token.line]);
	StringView line = FileInfoGetLine(fileInfo, token.line);

	fprintf(
		stderr,
		"%s:%d:%d: " TERM_COL_RED "error: " TERM_COL_RESET "%s"
		"\n%.*s\n"
		"%*s" TERM_COL_GREEN "^%*s" TERM_COL_RESET "\n",
		fileInfo->filename, token.line, lineOffset, message,
		line.length, line.chars,
		lineOffset, " ", token.text.length - 1, "~"
	);
}

static void emitCode(Compiler* compiler, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->textSection, format, args);
	va_end(args);
}

static void emitInstruction(Compiler* compiler, const char* format, ...)
{
	emitCode(compiler, "\t");
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->textSection, format, args);
	va_end(args);
	emitCode(compiler, "\n");
}

void emitData(Compiler* compiler, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->dataSection, format, args);
	va_end(args);
}

static void emitResult(Compiler* compiler, const Result* result)
{
	switch (result->locationType)
	{
		case RESULT_LOCATION_INT_CONSTANT:
			emitIntConstant(compiler, result);
			break;

		case RESULT_LOCATION_BASE_OFFSET:
			emitCode(compiler, "[rbp-%zx]", result->location.baseOffset);
			break;

		case RESULT_LOCATION_LABEL:
			emitCode(compiler, "[.L%d]", result->location.labelIndex);
			break;

		case RESULT_LOCATION_TEMP:
			emitCode(compiler, "[rbp-%zx]", compiler->temps.data[result->location.tempIndex]);
			break;

		default:
			ASSERT_NOT_REACHED();
	}
}

void emitIntConstant(Compiler* compiler, const Result* result)
{
	// Don't know if there is a difference between signed and unsigned here.
	switch (result->dataType.type)
	{
		case DATA_TYPE_CHAR:
			emitCode(compiler, "%d", result->location.charConstant);
			break;
		case DATA_TYPE_SHORT:
			emitCode(compiler, "%d", result->location.shortConstant);
			break;
		case DATA_TYPE_INT:
			emitCode(compiler, "%d", result->location.intConstant);
			break;
		case DATA_TYPE_LONG:
			emitCode(compiler, "%d", result->location.intConstant);
			break;
		case DATA_TYPE_LONG_LONG:
			emitCode(compiler, "%ll", result->location.longLongConstant);
			break;

		default:
			ASSERT_NOT_REACHED();
	}
}

static void emitMovToRegisterGp(Compiler* compiler, RegisterGp reg, const Result* result)
{
	emitCode(compiler, "\tmov %s, ", RegisterGpToString(reg, DataTypeSize(&result->dataType)));
	emitResult(compiler, result);
	emitCode(compiler, "\n");
}

static void emitMovFromRegisterGp(Compiler* compiler, const Result* result, RegisterGp reg)
{
	emitCode(compiler, "\tmov ");
	emitResult(compiler, result);
	emitCode(compiler, ", %s\n", RegisterGpToString(reg, DataTypeSize(&result->dataType)));
}

static void emitMovToRegisterSimd(Compiler* compiler, RegisterSimd reg, const Result* result)
{
	emitCode(compiler, "\tmov %s, ", RegisterSimdToString(reg));
	emitResult(compiler, result);
	emitCode(compiler, "\n");
}

static void emitMovFromRegisterSimd(Compiler* compiler, const Result* result, RegisterSimd reg)
{
	emitCode(compiler, "\tmov ");
	emitResult(compiler, result);
	emitCode(compiler, ", %s\n", RegisterSimdToString(reg));
}

static int allocateLabel(Compiler* compiler)
{
	return compiler->labelCount++;
}

static size_t allocateSingleVariableOnStack(Compiler* compiler, size_t size)
{
	// On x86 data in memory should be aligned to the size of the data.
	// If memory is not aligned the cpu might need to perform 2 loads
	// or with some instructions the program might crash.
	compiler->stackAllocationSize = ALIGN_UP_TO(size, compiler->stackAllocationSize) + size;
	return compiler->stackAllocationSize;
}

Result allocateTemp(Compiler* compiler, const DataType* dataType)
{
	Result result;
	result.locationType = RESULT_LOCATION_TEMP;
	result.dataType = *dataType;

	size_t tempSize = DataTypeSize(dataType);
	for (size_t i = 0; i < compiler->temps.size; i++)
	{
		if ((compiler->temps.data[i].isAllocated == false)
		 && (compiler->temps.data[i].size >= tempSize))
		{
			result.location.tempIndex = i;
			return result;
		}
	}

	Temp newTemp = {
		.baseOffset = allocateSingleVariableOnStack(compiler, tempSize),
		.size = tempSize,
		.isAllocated = true
	};
	result.location.tempIndex = compiler->temps.size;
	TempArrayAppend(&compiler->temps, newTemp);

	return result;
}

void freeTemp(Compiler* compiler, const Result* temp)
{
	compiler->temps.data[temp->location.tempIndex].isAllocated = false;
}

static int getIntegerConversionRank(const DataType* dataType)
{
	switch (dataType->type)
	{
		case DATA_TYPE_CHAR:	   return 1;
		case DATA_TYPE_SHORT:	   return 2;
		case DATA_TYPE_INT:		   return 3;
		case DATA_TYPE_LONG:	   return 4;
		case DATA_TYPE_LONG_LONG:  return 5;

		default:
			return -1;
	}
}

static DataType binaryExpressionGetResultingType(const DataType* a, const DataType* b)
{
	DataType result;

	if ((a->type == DATA_TYPE_LONG_DOUBLE) || (b->type == DATA_TYPE_LONG_DOUBLE))
	{
		result.type = DATA_TYPE_LONG_DOUBLE;
		return result;
	}

	if ((a->type == DATA_TYPE_DOUBLE) || (b->type == DATA_TYPE_DOUBLE))
	{
		result.type = DATA_TYPE_DOUBLE;
		return result;
	}

	if ((a->type == DATA_TYPE_FLOAT) || (b->type == DATA_TYPE_FLOAT))
	{
		result.type = DATA_TYPE_FLOAT;
		return result;
	}

	int aConversionRank = getIntegerConversionRank(a);
	int bConversionRank = getIntegerConversionRank(b);
	if ((aConversionRank == -1) || (bConversionRank == 1))
	{
		result.type = DATA_TYPE_ERROR;
		return result;
	}

	if (a->isUnsigned == b->isUnsigned)
	{
		result.type = (aConversionRank > bConversionRank) ? a->type : b->type;
		result.isUnsigned = a->isUnsigned;
		return result;
	}
	else
	{
		if ((a->isUnsigned) && (aConversionRank >= bConversionRank))
		{
			result.type = a->type;
			result.isUnsigned = true;
			return result;
		}
		else if ((b->isUnsigned) && (bConversionRank >= aConversionRank))
		{
			result.type = b->type;
			result.isUnsigned = true;
			return result;
		}
	}

	// Should have returned at integerConversionRank == -1
	ASSERT_NOT_REACHED();
		
	result.type = DATA_TYPE_ERROR;
	return result;
}

Result convertToType(Compiler* compiler, const Result* value, const DataType* type)
{
	if (value->dataType.type == type->type)
		return *value;
}

static Result compileExpr(Compiler* compiler, const Expr* expr)
{
	switch (expr->type)
	{
	case EXPR_NUMBER_LITERAL:
		return compileExprNumberLiteral(compiler, (ExprNumberLiteral*)expr);
	case EXPR_BINARY:
		return compileExprBinary(compiler, (ExprBinary*)expr);

	default:
		ASSERT_NOT_REACHED();
	}
}

static Result compileExprNumberLiteral(Compiler* compiler, const ExprNumberLiteral* expr)
{
	Result result;
	result.dataType = expr->dataType;

	switch (expr->dataType.type)
	{
		case DATA_TYPE_INT:
		case DATA_TYPE_LONG:
		case DATA_TYPE_LONG_LONG:
			result.locationType = RESULT_LOCATION_INT_CONSTANT;
			result.location.constant = strtoll(expr->literal.text.chars, NULL, 0);
			break;

		case DATA_TYPE_FLOAT:
			result.locationType = RESULT_LOCATION_LABEL;
			result.location.labelIndex = allocateLabel(compiler);
			emitData(
				compiler, ".L%d:\n\tdd %f\n",
				result.location.labelIndex, 
				strtof(expr->literal.text.chars, NULL)
			);
			break;

		case DATA_TYPE_DOUBLE:
			result.locationType = RESULT_LOCATION_LABEL;
			result.location.labelIndex = allocateLabel(compiler);
			emitData(
				compiler, ".L%d:\n\tdq %f\n",
				result.location.labelIndex,
				strtof(expr->literal.text.chars, NULL)
			);
			break;

		case DATA_TYPE_LONG_DOUBLE:
			break;
			
		default:
			ASSERT_NOT_REACHED();
	}

	return result;
}

static Result compileExprBinary(Compiler* compiler, const ExprBinary* expr)
{
	errorAt(compiler, expr->operator, "Test error");
	Result lhs = compileExpr(compiler, expr->left);
	Result rhs = compileExpr(compiler, expr->right);
	// Could check if the types are fundemental types
	DataType resultingType = binaryExpressionGetResultingType(&lhs.dataType, &rhs.dataType);
	if (resultingType.type == DATA_TYPE_ERROR)
	{
		
	}

	lhs = convertToType(compiler, &lhs, &resultingType);
	rhs = convertToType(compiler, &rhs, &resultingType);
	
	if ((lhs.locationType == RESULT_LOCATION_INT_CONSTANT) && (rhs.locationType == RESULT_LOCATION_INT_CONSTANT))
	{
		// return evaluateConstantBinaryExpression(compiler, &lhs, &rhs);
	}

	switch (expr->operator.type)
	{
		case TOKEN_PLUS:       return compileSimpleIntExprBinary(compiler, "add", &lhs, &rhs);
		case TOKEN_MINUS:      return compileSimpleIntExprBinary(compiler, "sub", &lhs, &rhs);
		case TOKEN_AMPERSAND:  return compileSimpleIntExprBinary(compiler, "and", &lhs, &rhs);
		case TOKEN_PIPE:	   return compileSimpleIntExprBinary(compiler, "or", &lhs, &rhs);
		case TOKEN_CIRCUMFLEX: return compileSimpleIntExprBinary(compiler, "xor", &lhs, &rhs);

		case TOKEN_ASTERISK:
		{
			if (resultingType.isUnsigned)
				{}
			else						
				return compileSimpleIntExprBinary(compiler, "imul", &lhs, &rhs);
		}

		default:
			ASSERT_NOT_REACHED();
	}
}

// Compiles operations that correspond to instructions with encoding op reg, reg.
// Requires lhs.dataType == rhs.dataType
static Result compileSimpleIntExprBinary(Compiler* compiler, const char* op, const Result* lhs, const Result* rhs)
{
	emitMovToRegisterGp(compiler, REGISTER_RAX, lhs);
	emitMovToRegisterGp(compiler, REGISTER_RBX, rhs);

	size_t resultSize = DataTypeSize(&lhs->dataType);

	emitInstruction(
		compiler, "%s %s, %s", op,
		RegisterGpToString(REGISTER_RAX, resultSize),
		RegisterGpToString(REGISTER_RBX, resultSize)
	);

	if (lhs->locationType == RESULT_LOCATION_TEMP) freeTemp(compiler, lhs);
	if (rhs->locationType == RESULT_LOCATION_TEMP) freeTemp(compiler, rhs);

	Result result = allocateTemp(compiler, &lhs->dataType);
	emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
	return result;
}

static void compileStmt(Compiler* compiler, const Stmt* stmt)
{
	switch (stmt->type)
	{
	case STMT_EXPRESSION:
		compileStmtExpression(compiler, (StmtExpression*)stmt);
		break;

	case STMT_RETURN:
		compileStmtReturn(compiler, (StmtReturn*)stmt);
		break;

	default:
		ASSERT_NOT_REACHED();
	}
}

static void compileStmtExpression(Compiler* compiler, const StmtExpression* stmt)
{
	compileExpr(compiler, stmt->expresssion);
}

static void compileStmtReturn(Compiler* compiler, const StmtReturn* stmt)
{
	if (stmt->returnValue != NULL)
	{
		Result returnValue = compileExpr(compiler, stmt->returnValue);
		emitMovToRegisterGp(compiler, REGISTER_RAX, &returnValue);
	}
}

String CompilerCompile(Compiler* compiler, const FileInfo* fileInfo, const StmtArray* ast)
{
	compiler->fileInfo = fileInfo;
	compiler->textSection = StringCopy("section .text\nglobal _start\n_start:\n\tmov rbp, rsp\n");
	compiler->dataSection = StringCopy("\nsection .data\n");
	compiler->stackAllocationSize = 0;
	compiler->labelCount = 0;

	TempArrayClear(&compiler->temps);
	for (size_t i = 0; i < ast->size; i++)
	{
		compileStmt(compiler, ast->data[i]);
	}

	emitInstruction(compiler, "mov rdi, rax");
	emitInstruction(compiler, "mov rax, 60");
	emitInstruction(compiler, "syscall");

	StringAppendLen(&compiler->textSection, compiler->dataSection.chars, compiler->dataSection.length);
	StringFree(&compiler->dataSection);

	return compiler->textSection;
}

static void copyTemp(Temp* dst, const Temp* src)
{
	*dst = *src;
}

ARRAY_TEMPLATE_DEFINITION(TempArray, Temp, copyTemp, NO_OP_FUNCTION)