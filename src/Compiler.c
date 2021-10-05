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

static void errorAt(Compiler* compiler, Token location, const char* format, ...);

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

static Result allocateGpRegister(Compiler* compiler)
{
	Result reg;
	reg.locationType = RESULT_LOCATION_REGISTER_GP;
	for (int i = 0; i < REGISTER_GP_COUNT; i++)
	{
		if (compiler->isRegisterGpAllocated.as.array[i] == false)
		{
			compiler->isRegisterGpAllocated.as.array[i] = true;
			reg.location.registerGp = i;
			return reg;
		}
	}

	reg.location.registerGp = REGISTER_ERROR;
	return reg;
}

static int allocateLabel(Compiler* compiler)
{
	return compiler->labelCount++;
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
	StringAppend(&compiler->textSection, "\n\t");
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->textSection, format, args);
	va_end(args);
}

static void emitText(Compiler* compiler, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->textSection, format, args);
	va_end(args);
}

static void emitData(Compiler* compiler, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	StringAppendVaFormat(&compiler->dataSection, format, args);
	va_end(args);
}

static void emitResultLocation(Compiler* compiler, Result result)
{
	switch (result.locationType)
	{
		case RESULT_LOCATION_BASE_OFFSET:
			StringAppendFormat(&compiler->textSection, " %s [rbp-%u]", dataSizeToString(DataTypeSize(result.dataType)), result.location.baseOffset);
			break;

		case RESULT_LOCATION_REGISTER_GP:
			StringAppendFormat(&compiler->textSection, " %s", RegisterGpToString(result.location.registerGp, DataTypeSize(result.dataType)));
			break;

			// chnage later // wont work with bigger immediates
		case RESULT_LOCATION_IMMEDIATE:
			StringAppendFormat(&compiler->textSection, " %d", result.location.immediate);
			break;

		case RESULT_LOCATION_LABEL:
			StringAppendFormat(&compiler->textSection, " [.L%d]", result.location.label);
			break;

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

static void emitTwoOperandInstruction(Compiler* compiler, const char* op, const Result* a, const Result* b)
{
	StringAppendFormat(&compiler->textSection, "\n\t%s", op);
	ASSERT(a->dataType.type == b->dataType.type);
	emitResultLocation(compiler, *a);
	emitText(compiler, ",");
	emitResultLocation(compiler, *b);
}

static Result ResultFromRegisterGp(RegisterGp reg, size_t size)
{
	Result result;
	result.locationType = RESULT_LOCATION_REGISTER_GP;
	result.location.registerGp = reg;

	switch (size)
	{
	case 1: result.dataType.type = DATA_TYPE_CHAR; break;
	case 2: result.dataType.type = DATA_TYPE_SHORT; break;
	case 4: result.dataType.type = DATA_TYPE_INT; break;
	case 8: result.dataType.type = DATA_TYPE_LONG_LONG; break;

	default:
		ASSERT_NOT_REACHED();
		break;
	}
	return result;
}

static Result compileExpr(Compiler* compiler, Expr* expr);

StringView removeNumberLiteralSuffix(StringView numberLiteral)
{
	const char* current = numberLiteral.chars + numberLiteral.length - 1;
	while ((((*current >= '0') && (*current <= '9')) || (*current == '.')) == false)
		current--;
	numberLiteral.length = current - numberLiteral.chars + 1;
	return numberLiteral;
}

static Result compileExprNumberLiteral(Compiler* compiler, ExprNumberLiteral* expr)
{
	Result result;
	result.dataType = expr->type; 
	if (expr->literal.type == TOKEN_INT_LITERAL)
	{
		result.locationType = RESULT_LOCATION_IMMEDIATE;
		// Don't know if this works or should there be a switch of types
		result.location.immediate = strtol(expr->literal.text.chars, NULL, 0);
	}
	else
	{
		result.locationType = RESULT_LOCATION_LABEL;
		result.location.label = allocateLabel(compiler);

		StringView literal = removeNumberLiteralSuffix(expr->literal.text);
		switch (result.dataType.type)
		{
			case DATA_TYPE_FLOAT:
				emitData(compiler, "\n.L%d:\n\tdd %.*s", result.location.label, literal.length, literal.chars);
				break;

			case DATA_TYPE_DOUBLE:
				emitData(compiler, "\n.L%d:\n\tdq %.*s", result.location.label, literal.length, literal.chars);
				break;

			case DATA_TYPE_LONG_DOUBLE:
				break;

			default:
				ASSERT_NOT_REACHED();
				break;
		}
	}

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
	Result reg = allocateGpRegister(compiler);

	if (reg.location.registerGp == -1)
		return false;

	result->locationType = RESULT_LOCATION_REGISTER_GP;
	result->location.registerGp = reg.location.registerGp;

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

	Result savedRegisterLhs = ResultFromRegisterGp(REGISTER_ERROR, SIZE_QWORD);
	Result tempLocationLhs = { 0, 0, 0 };

	if (lhs.locationType != RESULT_LOCATION_REGISTER_GP)
	{
		// Couldn't allocate register for lhs
		if (moveToRegister(compiler, lhs, &lhs) == false)
		{
			// Take any register to move the value into
			savedRegisterLhs = ResultFromRegisterGp(REGISTER_RAX, SIZE_QWORD);

			// Move the value to a temp memory location
			tempLocationLhs = allocateTemp(compiler);
			emitTwoOperandInstruction(compiler, "mov", &tempLocationLhs, &savedRegisterLhs);

			// Move lhs to register.
			emitTwoOperandInstruction(compiler, "mov", &savedRegisterLhs, &lhs);
			
			lhs.locationType = RESULT_LOCATION_REGISTER_GP;
			lhs.location.registerGp = savedRegisterLhs.location.registerGp;
		}
	}

	if (rhs.locationType != RESULT_LOCATION_REGISTER_GP)
	{
		if (moveToRegister(compiler, rhs, &rhs) == false)
		{
			Result scratchRegister = ResultFromRegisterGp(REGISTER_GP_SCRATCH, DataTypeSize(rhs.dataType));
			emitTwoOperandInstruction(compiler, "mov", &scratchRegister, &rhs);

			rhs.locationType = RESULT_LOCATION_REGISTER_GP;
			rhs.location.registerGp = REGISTER_GP_SCRATCH;
		}
	}

	const char* op = "";
	switch (operator)
	{
		case TOKEN_PLUS:       op = "add";  break;
		case TOKEN_MINUS:      op = "sub";  break;
		case TOKEN_AMPERSAND:  op = "and";  break;
		case TOKEN_PIPE:       op = "or";   break;
		case TOKEN_CIRCUMFLEX: op = "xor";  break;
		case TOKEN_ASTERISK:   op = "imul"; break;
			
		default:
			ASSERT_NOT_REACHED();
			break;
	}

	emitTwoOperandInstruction(compiler, op, &lhs, &rhs);

	if ((rhs.locationType == RESULT_LOCATION_REGISTER_GP) && (rhs.location.registerGp != REGISTER_GP_SCRATCH))
	{
		freeGpRegister(compiler, rhs.location.registerGp);
	}

	if (savedRegisterLhs.location.registerGp != REGISTER_ERROR)
	{
		Result scratchRegister = ResultFromRegisterGp(REGISTER_GP_SCRATCH, SIZE_QWORD);
		// Move the saved register to the sratch register
		emitTwoOperandInstruction(compiler, "mov", &scratchRegister, &tempLocationLhs);

		savedRegisterLhs.dataType = tempLocationLhs.dataType;
		// Move lhs to the temp location
		emitTwoOperandInstruction(compiler, "mov", &tempLocationLhs, &savedRegisterLhs);

		// Move the previous value of the register back to it
		emitTwoOperandInstruction(compiler, "mov", &savedRegisterLhs, &scratchRegister);
		
		// Add location type temp the size of the temp might be larger that the data stored in it
		lhs.locationType = RESULT_LOCATION_BASE_OFFSET;
		lhs.location.baseOffset = tempLocationLhs.location.baseOffset;
	}

	return lhs;
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

// Unit test this
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
		}
		else if ((b->isUnsigned) && (bConversionRank >= aConversionRank))
		{
			result.type = b->type;
			result.isUnsigned = true;
		}
	}

	result.type = DATA_TYPE_ERROR;

	return result;
}

static Result convertImmediate(Compiler* compiler, const DataType* type, const Result* value)
{
	Result result;
	result.dataType = *type;

	switch (type->type)
	{
		// Don't know if this works is conversion needed ?
		case DATA_TYPE_CHAR:
		case DATA_TYPE_SHORT:
		case DATA_TYPE_INT:
		case DATA_TYPE_LONG:
		case DATA_TYPE_LONG_LONG:
			result.location = value->location;
			result.locationType = value->locationType;
			break;

		case DATA_TYPE_DOUBLE:
		case DATA_TYPE_FLOAT:
		{
			int label = allocateLabel(compiler);
			emitData(compiler, "\n.L%d:\n\t", label);
			result.location.label = label;
			result.locationType = RESULT_LOCATION_LABEL;

			switch (type->type)
			{
				case DATA_TYPE_DOUBLE: emitData(compiler, "dq "); break;
				case DATA_TYPE_FLOAT: emitData(compiler, "dd "); break;
			}

			switch (value->dataType.type)
			{
			case DATA_TYPE_CHAR:
				if (value->dataType.isUnsigned) emitData(compiler, "%u.0", value->location.charImmediate);
				else emitData(compiler, "%d.0", value->location.charImmediate);
				break;

			case DATA_TYPE_SHORT:
				if (value->dataType.isUnsigned) emitData(compiler, "%u.0", value->location.shortImmediate);
				else emitData(compiler, "%d.0", value->location.shortImmediate);
				break;

			case DATA_TYPE_INT:
				if (value->dataType.isUnsigned) emitData(compiler, "%u.0", value->location.intImmediate);
				else emitData(compiler, "%d.0", value->location.intImmediate);
				break;


			case DATA_TYPE_LONG:
				if (value->dataType.isUnsigned) emitData(compiler, "%u.0", value->location.intImmediate);
				else emitData(compiler, "%d.0", value->location.intImmediate);
				break;


			case DATA_TYPE_LONG_LONG:
				if (value->dataType.isUnsigned) emitData(compiler, "%llu.0", value->location.intImmediate);
				else emitData(compiler, "%ll.0", value->location.intImmediate);
				break;

			default:
				ASSERT_NOT_REACHED();
			}

			break;
		}

		case DATA_TYPE_LONG_DOUBLE:
			break;

		default:
			ASSERT_NOT_REACHED();
	}

	return result;
}

static Result convertToType(Compiler* compiler, const DataType* type, const Result* value)
{
	if (value->dataType.type == type->type)
		return *value;

	if (DataTypeIsInt(&value->dataType) && value->locationType == RESULT_LOCATION_IMMEDIATE)
	{
		return convertImmediate(compiler, type, value);
	}

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

	DataType resultingType = binaryExpressionGetResultingType(&a.dataType, &b.dataType);
	if (resultingType.type == DATA_TYPE_ERROR)
	{
		errorAt(compiler, expr->operator, "Conversion error");
	}

	Result lhs = convertToType(compiler, &resultingType, &a);
	Result rhs = convertToType(compiler, &resultingType, &b);

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
		case TOKEN_ASTERISK:
		{
			if (resultingType.isUnsigned)
				{}
			else
				return compileSimpleIntBinaryExpression(compiler, lhs, rhs, expr->operator.type);

		}

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
		case EXPR_NUMBER_LITERAL: return compileExprNumberLiteral(compiler, (ExprNumberLiteral*)expr);
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
	LocalVariable variable;
	variable.baseOffset = allocateSingleVariableOnStack(compiler, DataTypeSize(stmt->type));
	variable.type = stmt->type;
	StringView variableName = stmt->name.text;
	LocalVariableTableSet(&compiler->locals, &variableName, variable);
	
	// if is integral data type
	if (stmt->initializer != NULL)
	{
		emitInstruction(compiler, "mov %s [rbp-%u],", dataSizeToString(DataTypeSize(variable.type)), variable.baseOffset);
		Result initializer = compileExpr(compiler, stmt->initializer);
		emitResultLocation(compiler, convertToType(compiler, &stmt->type, &initializer));
	}
	// Wont work with 2 memory operands
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
	compiler->textSection = StringCopy("section .text\nglobal _start\n_start:\n\tmov rbp, rsp");
	compiler->dataSection = StringCopy("\nsection .data");
	compiler->stackAllocationSize = 0;
	compiler->labelCount = 0;
	freeGpRegisters(compiler);

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
