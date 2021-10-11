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

// Maybe later add emit jump 
static void emitResult(Compiler* compiler, const Result* result);
static void emitIntConstant(Compiler* compiler, const Result* result);
static void emitMovToRegisterGp(Compiler* compiler, RegisterGp reg, const Result* result);
static void emitMovFromRegisterGp(Compiler* compiler, const Result* result, RegisterGp reg);
static void emitMovToRegisterSimd(Compiler* compiler, RegisterSimd reg, const Result* result);
static void emitMovFromRegisterSimd(Compiler* compiler, const Result* result, RegisterSimd reg);
static void emitSimdTypeName(Compiler* compiler, const DataType* type);

static void beginScope(Compiler* compiler);
static void endScope(Compiler* compiler);
static int allocateLabel(Compiler* compiler);
static size_t allocateSingleVariableOnStack(Compiler* compiler, size_t size);
static Result allocateTemp(Compiler* compiler, const DataType* dataType);
static void freeTemp(Compiler* compiler, const Result* temp);
static void freeIfIsTemp(Compiler* compiler, const Result* value);
// Returns false if the variable was already defined.
static bool declareLocalVariable(Compiler* compiler, StringView name, const DataType* type, Result* result);
static bool resolveLocalVariable(Compiler* compiler, StringView name, Result* result);

static const char* tokenTypeToCondition(TokenType token, bool isUnsigned);

static int getIntegerConversionRank(const DataType* dataType);
static DataType binaryExpressionGetResultingType(const DataType* a, const DataType* b);

static bool isResultLvalue(const Result* result);

static Result convertToType(Compiler* compiler, const Result* value, const DataType* type);

static Result compileExpr(Compiler* compiler, const Expr* expr);
static Result compileExprNumberLiteral(Compiler* compiler, const ExprNumberLiteral* expr);
// Maybe use this later for asignment operators like +=
static Result compileExprBinary(Compiler* compiler, const ExprBinary* expr);
static Result compileSimpleIntExprBinary(Compiler* compiler, const char* op, const Result* lhs, const Result* rhs);
static Result compileIntDivisionOrMultiplcation(Compiler* compiler, TokenType operator, const char* op, const Result* lhs, const Result* rhs);
static Result compileFloatExprBinary(Compiler* compiler, const char* op, const Result* lhs, const Result* rhs);
static Result compileComparasion(Compiler* compiler, Token op, const Result* lhs, const Result* rhs);
static Result compileAnd(Compiler* compiler, const Expr* lhs, const Expr* rhs);
static Result compileOr(Compiler* compiler, const Expr* lhs, const Expr* rhs);
static Result compileExprGrouping(Compiler* compiler, const ExprGrouping* expr);
static Result compileExprUnary(Compiler* compiler, const ExprUnary* expr);
static Result compileNegation(Compiler* compiler, const Result* operand);
static Result compileFloatNegation(Compiler* compiler, const Result* operand);
static Result compileExprIdentifier(Compiler* compiler, const ExprIdentifier* expr);
static Result compileExprAssignment(Compiler* compiler, const ExprAssignment* expr);
static void moveBetweenMemory(Compiler* compiler, const Result* lhs, const Result* rhs);

static void compileStmt(Compiler* compiler, const Stmt* stmt);
static void compileStmtExpression(Compiler* compiler, const StmtExpression* stmt);
static void compileStmtReturn(Compiler* compiler, const StmtReturn* stmt);
static void compileVariableDeclaration(Compiler* compiler, const StmtVariableDeclaration* stmt);
static void compileStmtBlock(Compiler* compiler, const StmtBlock* stmt);
static void compileStmtIf(Compiler* compiler, const StmtIf* stmt);
static void compileStmtWhileLoop(Compiler* compiler, const StmtWhileLoop* stmt);
static void compileStmtBreak(Compiler* compiler, const StmtBreak* stmt);
static void compileStmtContinue(Compiler* compiler, const StmtContinue* stmt);
static void compileStmtPutchar(Compiler* compiler, const StmtPutchar* stmt);

void CompilerInit(Compiler* compiler)
{
	//LocalVariableTableInit(&compiler->localVariables);
	TempArrayInit(&compiler->temps);
}

void CompilerFree(Compiler* compiler)
{
	//LocalVariableTableFree(&compiler->localVariables);
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
		"%s:%d:%d: " TERM_COL_RED "error: " TERM_COL_RESET,
		fileInfo->filename, token.line, lineOffset
	);

	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);

	fprintf(stderr,
		"\n%.*s\n"
		"%*s" TERM_COL_GREEN "^",
		line.length, line.chars,
		lineOffset, " "
	);

	for (size_t i = 0; i < token.text.length - 1; i++)
		fputc('~', stderr);

	fprintf(stderr, TERM_COL_RESET "\n");
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
	emitCode(compiler, "\n\t");
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->textSection, format, args);
	va_end(args);
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
			emitCode(
				compiler, "%s [rbp-%zu]",
				dataSizeToString(DataTypeSize(&result->dataType)), result->location.baseOffset
			);
			break;

		case RESULT_LOCATION_LABEL:
		case RESULT_LOCATION_LABEL_COSTANT:
			emitCode(compiler, "[.L%d]", result->location.labelIndex);
			break;

		case RESULT_LOCATION_TEMP:
			emitCode(
				compiler, "%s [rbp-%zu]",
				dataSizeToString(DataTypeSize(&result->dataType)), 
				compiler->temps.data[result->location.tempIndex].baseOffset
			);
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
			emitCode(compiler, "%lld", result->location.longLongConstant);
			break;

		default:
			ASSERT_NOT_REACHED();
	}
}

static void emitMovToRegisterGp(Compiler* compiler, RegisterGp reg, const Result* result)
{
	emitInstruction(compiler, "mov %s, ", RegisterGpToString(reg, DataTypeSize(&result->dataType)));
	emitResult(compiler, result);
}

static void emitMovFromRegisterGp(Compiler* compiler, const Result* result, RegisterGp reg)
{
	emitInstruction(compiler, "mov ");
	emitResult(compiler, result);
	emitCode(compiler, ", %s", RegisterGpToString(reg, DataTypeSize(&result->dataType)));
}

static void emitMovToRegisterSimd(Compiler* compiler, RegisterSimd reg, const Result* result)
{
	switch (DataTypeSize(&result->dataType))
	{
		case SIZE_DWORD: emitInstruction(compiler, "movss %s, ", RegisterSimdToString(reg)); break;
		case SIZE_QWORD: emitInstruction(compiler, "movsd %s, ", RegisterSimdToString(reg)); break;
		default:
			ASSERT_NOT_REACHED();
	}
	emitResult(compiler, result);
}

static void emitMovFromRegisterSimd(Compiler* compiler, const Result* result, RegisterSimd reg)
{
	switch (DataTypeSize(&result->dataType))
	{
		case SIZE_DWORD: emitInstruction(compiler, "movss "); break;
		case SIZE_QWORD: emitInstruction(compiler, "movsd "); break;
		default:
			ASSERT_NOT_REACHED();
	}
	emitResult(compiler, result);
	emitCode(compiler, ", %s", RegisterSimdToString(reg));
}

void emitSimdTypeName(Compiler* compiler, const DataType* type)
{
	switch (type->type)
	{
		case DATA_TYPE_DOUBLE:
			emitCode(compiler, "sd");
			break;
		case DATA_TYPE_FLOAT:
			emitCode(compiler, "ss");
			break;
		// This is bad
		default:
			emitCode(compiler, "si");
			break;
	}
}

void beginScope(Compiler* compiler)
{
	Scope scope;
	LocalVariableTableInit(&scope.localVariables);
	if (compiler->currentScope == NULL)
	{
		scope.enclosing = NULL;
		compiler->currentScope = &scope;
	}
	else
	{
		scope.enclosing = compiler->currentScope;
		compiler->currentScope = &scope;
	}
}

void endScope(Compiler* compiler)
{
	LocalVariableTableFree(&compiler->currentScope->localVariables);
	compiler->currentScope = compiler->currentScope->enclosing;
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
			compiler->temps.data[i].isAllocated = true;
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

static void freeTemp(Compiler* compiler, const Result* temp)
{
	compiler->temps.data[temp->location.tempIndex].isAllocated = false;
}

static void freeIfIsTemp(Compiler* compiler, const Result* value)
{
	if (value->locationType == RESULT_LOCATION_TEMP)
	{
		freeTemp(compiler, value);
	}
}

static bool declareLocalVariable(Compiler* compiler, StringView name, const DataType* type, Result* result)
{
	//if (compiler->currentScope == NULL)
	//{
	//	GlobalVariable;
	//}
	ASSERT(compiler->currentScope != NULL);

	LocalVariable variable;
	if (LocalVariableTableGet(&compiler->currentScope->localVariables, &name, &variable))
	{
		return false;
	}
	else
	{
		// If is not array
		variable.dataType = *type;
		variable.baseOffset = allocateSingleVariableOnStack(compiler, DataTypeSize(type));
		result->dataType = *type;
		result->locationType = RESULT_LOCATION_BASE_OFFSET;
		result->location.baseOffset = variable.baseOffset;
		// LocalVariable could just store a result
		LocalVariableTableSet(&compiler->currentScope->localVariables, &name, variable);
		return true;
	}
}

bool resolveLocalVariable(Compiler* compiler, StringView name, Result* result)
{
	LocalVariable local;

	Scope* scope = compiler->currentScope;
	while (scope != NULL)
	{
		if (LocalVariableTableGet(&scope->localVariables, &name, &local))
		{
			result->dataType = local.dataType;
			result->locationType = RESULT_LOCATION_BASE_OFFSET;
			result->location.baseOffset = local.baseOffset;
			return true;
		}

		scope = scope->enclosing;
	}

	return false;
}

const char* tokenTypeToCondition(TokenType token, bool isUnsigned)
{
	switch (token)
	{
		case TOKEN_EQUALS_EQUALS: return "e";
		case TOKEN_BANG_EQUALS: return "ne";
		case TOKEN_LESS_THAN: return isUnsigned ? "b" : "l";
		case TOKEN_LESS_THAN_EQUALS: return isUnsigned ? "be" : "le";
		case TOKEN_MORE_THAN: return isUnsigned ? "a" : "g";
		case TOKEN_MORE_THAN_EQUALS: return isUnsigned ? "ae" : "ge";

		default:
			ASSERT_NOT_REACHED();
			return "";
	}
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
	if ((aConversionRank == -1) || (bConversionRank == -1))
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
		// Simplify this later if the signed type has higher conversion rank
		else if ((b->isUnsigned))
		{
			result.type = a->type;
			result.isUnsigned = false;
			return result;
		}
		else if ((a->isUnsigned))
		{
			result.type = b->type;
			result.isUnsigned = false;
			return result;
		}
	}

	// Should have returned at integerConversionRank == -1
	ASSERT_NOT_REACHED();
		
	result.type = DATA_TYPE_ERROR;
	return result;
}

bool isResultLvalue(const Result* result)
{
	return (result->locationType == RESULT_LOCATION_BASE_OFFSET)
		|| (result->locationType == RESULT_LOCATION_LABEL);
}

Result convertToType(Compiler* compiler, const Result* value, const DataType* type)
{
	if (value->locationType == RESULT_LOCATION_INT_CONSTANT)
	{
		// evaluateConstantConversion(compiler, value, type);
	}

	Result result;
	result = *value;
	result.dataType = *type;

	size_t valueTypeSize = DataTypeSize(&value->dataType);
	size_t resultTypeSize = DataTypeSize(type);

	if (DataTypeIsInt(type) && DataTypeIsInt(&value->dataType))
	{
		// If the resulting type is smaller or equal just take the lower bytes.
		if (valueTypeSize >= resultTypeSize)
		{
			return result;
		}
		// Conversion from unsigned to signed and unsigned to unsigned types.
		else if (value->dataType.isUnsigned)
		{
			// mov to 32 bit register zero extends the upper part.
			if ((valueTypeSize == SIZE_DWORD) && (resultTypeSize == SIZE_QWORD))
			{
				emitMovToRegisterGp(compiler, REGISTER_RAX, value);
				freeIfIsTemp(compiler, value);
				result = allocateTemp(compiler, type);
				emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
				return result;
			}
			else
			{
				//emitInstruction(compiler, "movzx %s, ", RegisterGpToString(REGISTER_RAX, resultTypeSize));
				//emitResult(compiler, value);
				emitMovToRegisterGp(compiler, REGISTER_RAX, value);
				emitInstruction(
					compiler, "movzx rax, %s",
					RegisterGpToString(REGISTER_RAX, valueTypeSize)
				);

				freeIfIsTemp(compiler, value);
				result = allocateTemp(compiler, type);
				emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
				return result;
			}
		}
		else if (value->dataType.isUnsigned == false)
		{
			// movsx and movzx don't take immediate operands
			//emitInstruction(compiler, "movsx %s, ", RegisterGpToString(REGISTER_RAX, resultTypeSize));
			//emitResult(compiler, value);

			emitMovToRegisterGp(compiler, REGISTER_RAX, value);
			emitInstruction(
				compiler, "movsx rax, %s",
				RegisterGpToString(REGISTER_RAX, valueTypeSize)
			);

			freeIfIsTemp(compiler, value);
			result = allocateTemp(compiler, type);
			emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
			return result;
		}
	}
	if (DataTypeIsFloat(type) || DataTypeIsFloat(&value->dataType))
	{
		if (value->dataType.type == type->type)
			return result;

		if (DataTypeIsFloat(&value->dataType) && DataTypeIsFloat(type))
		{
			emitMovToRegisterSimd(compiler, REGISTER_XMM2, value);
			
			emitInstruction(compiler, "cvt");
			emitSimdTypeName(compiler, &value->dataType);
			emitCode(compiler, "2");
			emitSimdTypeName(compiler, type);
			emitCode(compiler, " xmm1, xmm2");

			freeIfIsTemp(compiler, value);
			result = allocateTemp(compiler, type);
			emitMovFromRegisterSimd(compiler, &result, REGISTER_XMM1);
			return result;
		}
		else if (DataTypeIsInt(&value->dataType) && DataTypeIsFloat(type))
		{
			// Move because you can't movsx immediate operands
			emitMovToRegisterGp(compiler, REGISTER_RAX, value);
			// cvt instructions require the operand to be 32 or 64 bits
			// could do a check for dword and qword here to avoid mov but it probably doesn't matter
			emitInstruction(
				compiler, "movsx rax, %s",
				RegisterGpToString(REGISTER_RAX, valueTypeSize)
			);

			emitInstruction(compiler, "cvt");
			emitSimdTypeName(compiler, &value->dataType);
			emitCode(compiler, "2");
			emitSimdTypeName(compiler, type);
			emitCode(compiler, " xmm1, rax");

			freeIfIsTemp(compiler, value);
			result = allocateTemp(compiler, type);
			emitMovFromRegisterSimd(compiler, &result, REGISTER_XMM1);
			return result;
		}
		else if (DataTypeIsFloat(&value->dataType) && DataTypeIsInt(type))
		{
			emitMovToRegisterSimd(compiler, REGISTER_XMM1, value);

			emitInstruction(compiler, "cvt");
			emitSimdTypeName(compiler, &value->dataType);
			emitCode(compiler, "2");
			emitSimdTypeName(compiler, type);
			emitCode(compiler, " rax, xmm1");

			freeIfIsTemp(compiler, value);
			result = allocateTemp(compiler, type);
			emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
			return result;
		}
	}

	// Maybe just throw an error here later
	ASSERT_NOT_REACHED();

	return result;
}

static Result compileExpr(Compiler* compiler, const Expr* expr)
{
	switch (expr->type)
	{
		case EXPR_NUMBER_LITERAL:
			return compileExprNumberLiteral(compiler, (ExprNumberLiteral*)expr);
		case EXPR_BINARY:
			return compileExprBinary(compiler, (ExprBinary*)expr);
		case EXPR_GROUPING:
			return compileExprGrouping(compiler, (ExprGrouping*)expr);
		case EXPR_UNARY:
			return compileExprUnary(compiler, (ExprUnary*)expr);
		case EXPR_IDENTIFIER:
			return compileExprIdentifier(compiler, (ExprIdentifier*)expr);
		case EXPR_ASSIGNMENT:
			return compileExprAssignment(compiler, (ExprAssignment*)expr);

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
		case DATA_TYPE_CHAR:
			result.locationType = RESULT_LOCATION_INT_CONSTANT;
			result.location.constant = expr->literal.text.chars[1];
			break;

		case DATA_TYPE_INT:
		case DATA_TYPE_LONG:
		case DATA_TYPE_LONG_LONG:
			result.locationType = RESULT_LOCATION_INT_CONSTANT;
			result.location.constant = strtoll(expr->literal.text.chars, NULL, 0);
			break;

		case DATA_TYPE_FLOAT:
			result.locationType = RESULT_LOCATION_LABEL_COSTANT;
			result.location.labelIndex = allocateLabel(compiler);
			emitData(
				compiler, ".L%d:\n\tdd %f\n",
				result.location.labelIndex, 
				strtof(expr->literal.text.chars, NULL)
			);
			break;

		case DATA_TYPE_DOUBLE:
			result.locationType = RESULT_LOCATION_LABEL_COSTANT;
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
	if (expr->operator.type == TOKEN_AMPERSAND_AMPERSAND)
	{
		return compileAnd(compiler, expr->left, expr->right);
	}
	else if (expr->operator.type == TOKEN_PIPE_PIPE)
	{
		return compileOr(compiler, expr->left, expr->right);
	}

	Result lhs = compileExpr(compiler, expr->left);
	Result rhs = compileExpr(compiler, expr->right);
	// Could check if the types are fundemental types
	DataType resultType = binaryExpressionGetResultingType(&lhs.dataType, &rhs.dataType);
	if (resultType.type == DATA_TYPE_ERROR)
	{
		//errorAt()
		//return;
	}

	lhs = convertToType(compiler, &lhs, &resultType);
	rhs = convertToType(compiler, &rhs, &resultType);
	
	if ((lhs.locationType == RESULT_LOCATION_INT_CONSTANT) && (rhs.locationType == RESULT_LOCATION_INT_CONSTANT))
	{
		// return evaluateConstantBinaryExpression(compiler, &lhs, &rhs);
	}

	switch (expr->operator.type)
	{
		case TOKEN_PLUS:       
		{
			if (DataTypeIsInt(&resultType))
				return compileSimpleIntExprBinary(compiler, "add", &lhs, &rhs);
			else if (DataTypeIsFloat(&resultType))
				return compileFloatExprBinary(compiler, "add", &lhs, &rhs);
		}
		case TOKEN_MINUS:     
		{
			if (DataTypeIsInt(&resultType))
				return compileSimpleIntExprBinary(compiler, "sub", &lhs, &rhs);
			else if (DataTypeIsFloat(&resultType))
				return compileFloatExprBinary(compiler, "sub", &lhs, &rhs);
		}
		case TOKEN_AMPERSAND:  return compileSimpleIntExprBinary(compiler, "and", &lhs, &rhs);
		case TOKEN_PIPE:	   return compileSimpleIntExprBinary(compiler, "or", &lhs, &rhs);
		case TOKEN_CIRCUMFLEX: return compileSimpleIntExprBinary(compiler, "xor", &lhs, &rhs);

		case TOKEN_PERCENT:
		case TOKEN_SLASH:
		{
			if (DataTypeIsInt(&resultType))
			{
				if (resultType.isUnsigned)
					return compileIntDivisionOrMultiplcation(compiler, expr->operator.type, "div", &lhs, &rhs);
				else
					return compileIntDivisionOrMultiplcation(compiler, expr->operator.type, "idiv", &lhs, &rhs);
			}
			else if (DataTypeIsFloat(&resultType))
			{
				return compileFloatExprBinary(compiler, "div", &lhs, &rhs);
			}
			break;
		}

		case TOKEN_ASTERISK:
		{
			if (DataTypeIsInt(&resultType))
			{
				if (resultType.isUnsigned)
					return compileIntDivisionOrMultiplcation(compiler, expr->operator.type, "mul", &lhs, &rhs);
				else
					return compileIntDivisionOrMultiplcation(compiler, expr->operator.type, "imul", &lhs, &rhs);
			}
			else if (DataTypeIsFloat(&resultType))
			{
				return compileFloatExprBinary(compiler, "mul", &lhs, &rhs);
			}
			break;
		}

		case TOKEN_LESS_THAN:
		case TOKEN_LESS_THAN_EQUALS:
		case TOKEN_MORE_THAN:
		case TOKEN_MORE_THAN_EQUALS:
		case TOKEN_EQUALS_EQUALS:
		case TOKEN_BANG_EQUALS:
		{
			return compileComparasion(compiler, expr->operator, &lhs, &rhs);
			break;
		}
	}

	ASSERT_NOT_REACHED();
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

	freeIfIsTemp(compiler, lhs);
	freeIfIsTemp(compiler, rhs);

	Result result = allocateTemp(compiler, &lhs->dataType);
	emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
	return result;
}

static Result compileIntDivisionOrMultiplcation(Compiler* compiler, TokenType operator, const char* op, const Result* lhs, const Result* rhs)
{
	// Write a not about divison here
	emitInstruction(compiler, "xor rdx, rdx");

	emitMovToRegisterGp(compiler, REGISTER_RAX, lhs);
	emitMovToRegisterGp(compiler, REGISTER_RBX, rhs);

	size_t resultSize = DataTypeSize(&lhs->dataType);

	emitInstruction(compiler, "%s %s", op, RegisterGpToString(REGISTER_RBX, resultSize));

	freeIfIsTemp(compiler, lhs);
	freeIfIsTemp(compiler, rhs);

	Result result = allocateTemp(compiler, &lhs->dataType);

	switch (operator)
	{
		case TOKEN_SLASH:
		case TOKEN_ASTERISK:
			emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
			break;

		case TOKEN_PERCENT:
		{
			if (DataTypeSize(&lhs->dataType) == SIZE_BYTE)
			{
				emitInstruction(compiler, "mov ");
				emitResult(compiler, &result);
				emitCode(compiler, ", ");
				emitCode(compiler, "ah");
			}
			else
			{
				emitMovFromRegisterGp(compiler, &result, REGISTER_RDX);
			}
			break;
		}

		default:
			ASSERT_NOT_REACHED();
			break;
	}

	return result;
}

Result compileFloatExprBinary(Compiler* compiler, const char* op, const Result* lhs, const Result* rhs)
{
	emitMovToRegisterSimd(compiler, REGISTER_XMM1, lhs);
	emitMovToRegisterSimd(compiler, REGISTER_XMM2, rhs);

	size_t resultSize = DataTypeSize(&lhs->dataType);

	emitInstruction(compiler, "%s", op);
	switch (resultSize)
	{
		case SIZE_DWORD: emitCode(compiler, "ss"); break;
		case SIZE_QWORD: emitCode(compiler, "sd"); break;

		default:
			ASSERT_NOT_REACHED();
	}
	emitCode(compiler, " xmm1, xmm2");

	freeIfIsTemp(compiler, lhs);
	freeIfIsTemp(compiler, rhs);

	Result result = allocateTemp(compiler, &lhs->dataType);
	emitMovFromRegisterSimd(compiler, &result, REGISTER_XMM1);
	return result;
}

Result compileComparasion(Compiler* compiler, Token op, const Result* lhs, const Result* rhs)
{
	if (DataTypeIsFloat(&lhs->dataType))
	{
		// https://stackoverflow.com/questions/8627331/what-does-ordered-unordered-comparison-mean
		emitMovToRegisterSimd(compiler, REGISTER_XMM1, lhs);
		emitMovToRegisterSimd(compiler, REGISTER_XMM2, rhs);

		size_t resultSize = DataTypeSize(&lhs->dataType);

		emitInstruction(compiler, "comi");
		emitSimdTypeName(compiler, &lhs->dataType),

		emitCode(
			compiler, " %s, %s",
			RegisterSimdToString(REGISTER_XMM1),
			RegisterSimdToString(REGISTER_XMM2)
		);
		emitInstruction(compiler, "set%s al", tokenTypeToCondition(op.type, true));
	}
	else if (DataTypeIsInt(&lhs->dataType))
	{
		emitMovToRegisterGp(compiler, REGISTER_RAX, lhs);
		emitMovToRegisterGp(compiler, REGISTER_RBX, rhs);

		size_t resultSize = DataTypeSize(&lhs->dataType);

		emitInstruction(
			compiler, "cmp %s, %s",
			RegisterGpToString(REGISTER_RAX, resultSize),
			RegisterGpToString(REGISTER_RBX, resultSize)
		);
		emitInstruction(compiler, "set%s al", tokenTypeToCondition(op.type, lhs->dataType.isUnsigned));
	}

	freeIfIsTemp(compiler, lhs);
	freeIfIsTemp(compiler, rhs);

	// Don't know what should the return type be.
	DataType integer;
	integer.isUnsigned = false;
	integer.type = DATA_TYPE_INT;
	Result result = allocateTemp(compiler, &integer);
	emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
	return result;
}

Result compileAnd(Compiler* compiler, const Expr* left, const Expr* right)
{
	// Should probably use movzx here so smaller operands also work
	Result lhs = compileExpr(compiler, left);
	emitMovToRegisterGp(compiler, REGISTER_RAX, &lhs);
	emitInstruction(compiler, "cmp rax, 0");
	int endLabel = allocateLabel(compiler);
	emitInstruction(compiler, "je .L%d", endLabel);
	freeIfIsTemp(compiler, &lhs);
	Result rhs = compileExpr(compiler, right);
	emitMovToRegisterGp(compiler, REGISTER_RAX, &rhs);
	emitInstruction(compiler, "cmp rax, 0");
	emitCode(compiler, "\n.L%d:", endLabel);
	emitInstruction(compiler, "setne al");
	freeIfIsTemp(compiler, &rhs);
	// This is probably incorrect
	Result result = allocateTemp(compiler, &rhs.dataType);
	emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
	return result;
}

Result compileOr(Compiler* compiler, const Expr* left, const Expr* right)
{
	// Should probably use movzx here so smaller operands also work
	Result lhs = compileExpr(compiler, left);
	emitMovToRegisterGp(compiler, REGISTER_RAX, &lhs);
	emitInstruction(compiler, "cmp rax, 0");
	int endLabel = allocateLabel(compiler);
	emitInstruction(compiler, "jne .L%d", endLabel);
	freeIfIsTemp(compiler, &lhs);
	Result rhs = compileExpr(compiler, right);
	emitMovToRegisterGp(compiler, REGISTER_RAX, &rhs);
	emitInstruction(compiler, "cmp rax, 0");
	emitCode(compiler, "\n.L%d:", endLabel);
	emitInstruction(compiler, "setne al");
	freeIfIsTemp(compiler, &rhs);
	// This is probably incorrect
	Result result = allocateTemp(compiler, &rhs.dataType);
	emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
	return result;
}

static Result compileExprGrouping(Compiler* compiler, const ExprGrouping* expr)
{
	return compileExpr(compiler, expr->expression);
}

Result compileExprUnary(Compiler* compiler, const ExprUnary* expr)
{
	Result operand = compileExpr(compiler, expr->operand);

	switch (expr->operator)
	{
		case TOKEN_MINUS:
		{
			if (DataTypeIsInt(&operand.dataType))
				return compileNegation(compiler, &operand);
			else if (DataTypeIsFloat(&operand.dataType))
				return compileFloatNegation(compiler, &operand);
		}

		case TOKEN_PLUS:
			break;

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

static Result compileNegation(Compiler* compiler, const Result* operand)
{
	emitMovToRegisterGp(compiler, REGISTER_RAX, operand);
	emitInstruction(compiler, "neg rax");

	if (operand->locationType == RESULT_LOCATION_TEMP)
		freeTemp(compiler, operand);

	Result result = allocateTemp(compiler, &operand->dataType);
	emitMovFromRegisterGp(compiler, &result, REGISTER_RAX);
	return result;
}

Result compileFloatNegation(Compiler* compiler, const Result* operand)
{
	// This could be done faster with xorps and a bitmask
	emitMovToRegisterSimd(compiler, REGISTER_XMM1, operand);
	emitInstruction(compiler, "pxor xmm0, xmm0");
	if (operand->dataType.type == DATA_TYPE_FLOAT)
	{
		emitInstruction(compiler, "subss xmm0, xmm1");
	}
	else if (operand->dataType.type == DATA_TYPE_DOUBLE)
	{
		emitInstruction(compiler, "subsd xmm0, xmm1");
	}
	else
	{
		ASSERT_NOT_REACHED();
	}
	Result result = allocateTemp(compiler, &operand->dataType);
	emitMovFromRegisterSimd(compiler, &result, REGISTER_XMM0);
	freeIfIsTemp(compiler, operand);
	return result;
}

Result compileExprIdentifier(Compiler* compiler, const ExprIdentifier* expr)
{
	Result result;
	if (resolveLocalVariable(compiler, expr->name.text, &result) == false)
	{
		errorAt(
			compiler, expr->name,
			"undeclared variable '%.*s' used", expr->name.text.length, expr->name.text.chars
		);
	}

	return result;
}

Result compileExprAssignment(Compiler* compiler, const ExprAssignment* expr)
{
	Result lhs = compileExpr(compiler, expr->left);
	if (isResultLvalue(&lhs) == false)
	{
		errorAt(compiler, expr->operator, "cannot asign to a non lvalue");
		//return;
	}
	
	Result rhs = compileExpr(compiler, expr->right);
	rhs = convertToType(compiler, &rhs, &lhs.dataType);
	moveBetweenMemory(compiler, &lhs, &rhs);

	Result result = allocateTemp(compiler, &lhs.dataType);
	moveBetweenMemory(compiler, &result, &rhs);
	// https://en.cppreference.com/w/c/language/operator_assignment

	freeIfIsTemp(compiler, &lhs);
	freeIfIsTemp(compiler, &rhs);
	return result;
}

// Requiers type to be the same maybe later add assert
static void moveBetweenMemory(Compiler* compiler, const Result* lhs, const Result* rhs)
{
	//if (DataTypeIsFloat(&lhs->dataType))
	//{
	//	emitMovFromRegisterSimd(compiler, REGISTER_XMM0, lhs);
	//	emitMovFromRegisterSimd(compiler, rhs, REGISTER_XMM0);
	//}

	// Type shouldn't matter in this case
	emitMovToRegisterGp(compiler, REGISTER_RAX, rhs);
	emitMovFromRegisterGp(compiler, lhs, REGISTER_RAX);
	
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

	case STMT_VARIABLE_DECLARATION:
		compileVariableDeclaration(compiler, (StmtVariableDeclaration*)stmt);
		break;

	case STMT_BLOCK:
		compileStmtBlock(compiler, (StmtBlock*)stmt);
		break;

	case STMT_IF:
		compileStmtIf(compiler, (StmtIf*)stmt);
		break;

	case STMT_WHILE_LOOP:
		compileStmtWhileLoop(compiler, (StmtWhileLoop*)stmt);
		break;

	case STMT_BREAK:
		compileStmtBreak(compiler, (StmtBreak*)stmt);
		break;

	case STMT_CONTINUE:
		compileStmtContinue(compiler, (StmtContinue*)stmt);
		break;

	case STMT_PUTCHAR:
		compileStmtPutchar(compiler, (StmtPutchar*)stmt);
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
		if (DataTypeIsInt(&returnValue.dataType))
		{
			emitMovToRegisterGp(compiler, REGISTER_RAX, &returnValue);
		}
		else if (DataTypeIsFloat(&returnValue.dataType))
		{
			emitMovToRegisterSimd(compiler, REGISTER_XMM0, &returnValue);
		}
	}
	//emitInstruction(compiler, "ret");
}

void compileVariableDeclaration(Compiler* compiler, const StmtVariableDeclaration* stmt)
{
	Result variable;
	if (declareLocalVariable(compiler, stmt->name.text, &stmt->dataType, &variable) == false)
	{
		errorAt(
			compiler, stmt->name,
			"redeclaration of variable '%.*s'", stmt->name.text.length, stmt->name.text.chars
		);
		return;
	}

	// https://en.cppreference.com/w/c/language/declarations

	if (stmt->initializer != NULL)
	{
		Result initializer = compileExpr(compiler, stmt->initializer);
		initializer = convertToType(compiler, &initializer, &stmt->dataType);
		moveBetweenMemory(compiler, &variable, &initializer);
	}
}

void compileStmtBlock(Compiler* compiler, const StmtBlock* stmt)
{
	//beginScope(compiler);
	Scope scope;
	LocalVariableTableInit(&scope.localVariables);
	if (compiler->currentScope == NULL)
	{
		scope.enclosing = NULL;
		compiler->currentScope = &scope;
	}
	else
	{
		scope.enclosing = compiler->currentScope;
		compiler->currentScope = &scope;
	}

	for (size_t i = 0; i < stmt->satements.size; i++)
	{
		compileStmt(compiler, stmt->satements.data[i]);
	}

	//endScope(compiler);
	LocalVariableTableFree(&compiler->currentScope->localVariables);
	compiler->currentScope = compiler->currentScope->enclosing;
}

static void compileStmtIf(Compiler* compiler, const StmtIf* stmt)
{
	Result condition = compileExpr(compiler, stmt->condition);
	emitMovToRegisterGp(compiler, REGISTER_RAX, &condition);
	freeIfIsTemp(compiler, &condition);
	int elseLabel = allocateLabel(compiler);
	int endLabel = allocateLabel(compiler);
	emitInstruction(compiler, "cmp rax, 0");
	emitInstruction(compiler, "je .L%d", elseLabel);
	compileStmt(compiler, stmt->thenBlock);
	emitInstruction(compiler, "jmp .L%d", endLabel);
	emitCode(compiler, "\n.L%d:", elseLabel);
	if (stmt->elseBlock != NULL)
	{
		compileStmt(compiler, stmt->elseBlock);
	}
	emitCode(compiler, "\n.L%d:", endLabel);
}

void compileStmtWhileLoop(Compiler* compiler, const StmtWhileLoop* stmt)
{
	Loop loop;
	if (compiler->currentLoop == NULL)
		loop.enclosing = NULL;
	else
		loop.enclosing = compiler->currentLoop;
	compiler->currentLoop = &loop;


	int endLabel = allocateLabel(compiler);
	int startLabel = allocateLabel(compiler);
	loop.loopEnd = endLabel;
	loop.loopStart = startLabel;
	emitCode(compiler, "\n.L%d:", startLabel);
	Result condition = compileExpr(compiler, stmt->condition);
	emitMovToRegisterGp(compiler, REGISTER_RAX, &condition);
	emitInstruction(compiler, "cmp rax, 0");
	emitInstruction(compiler, "je .L%d", endLabel);
	compileStmt(compiler, stmt->body);
	emitInstruction(compiler, "jmp .L%d", startLabel);

	emitCode(compiler, "\n.L%d:", endLabel);

	compiler->currentLoop = compiler->currentLoop->enclosing;
}

void compileStmtBreak(Compiler* compiler, const StmtBreak* stmt)
{
	if (compiler->currentLoop == NULL)
	{
		errorAt(compiler, stmt->token, "break statments only allowed inside loops");
		return;
	}
	emitInstruction(compiler, "jmp .L%d", compiler->currentLoop->loopEnd);
}

void compileStmtContinue(Compiler* compiler, const StmtContinue* stmt)
{
	if (compiler->currentLoop == NULL)
	{
		errorAt(compiler, stmt->token, "continue statments only allowed inside loops");
		return;
	}
	emitInstruction(compiler, "jmp .L%d", compiler->currentLoop->loopStart);
}

void compileStmtPutchar(Compiler* compiler, const StmtPutchar* stmt)
{
	Result argument = compileExpr(compiler, stmt->expresssion);
	DataType charType;
	charType.type = DATA_TYPE_CHAR;
	charType.isUnsigned = true;
	argument = convertToType(compiler, &argument, &charType);
	Result arg = allocateTemp(compiler, &charType);
	freeIfIsTemp(compiler, &argument);
	moveBetweenMemory(compiler, &arg, &argument);
	emitInstruction(compiler, "mov rax, 1"); 
	emitInstruction(compiler, "mov rdi, 1"); 
	emitInstruction(compiler, "lea rsi, [rbp-%d]", compiler->temps.data[arg.location.tempIndex].baseOffset);
	emitInstruction(compiler, "mov rdx, 1"); 
	emitInstruction(compiler, "syscall"); 
}

String CompilerCompile(Compiler* compiler, const FileInfo* fileInfo, const StmtArray* ast)
{
	compiler->fileInfo = fileInfo;
	compiler->textSection = StringCopy("section .text\nglobal _start\n_start:\n\tmov rbp, rsp");
	compiler->dataSection = StringCopy("\nsection .data\n");
	compiler->stackAllocationSize = 0;
	compiler->labelCount = 0;
	compiler->hadError = false;
	compiler->currentScope = NULL;
	compiler->currentLoop = NULL;


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