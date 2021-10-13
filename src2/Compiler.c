#include "Compiler.h"
#include "TerminalColors.h"
#include "Assert.h"

Compiler CompilerInit(Compiler* compiler)
{
	
}

void CompilerFree(Compiler* compiler)
{

}

String CompilerCompile(Compiler* compiler, const TokenArray* tokens, const FileInfo* fileInfo)
{
	compiler->tokens = tokens;
	compiler->fileInfo = fileInfo;

	compiler->currentTokenIndex = 0;
	compiler->hadError = false;
	compiler->isSynchronizing = false;

	compiler->textSection = StringCopy("");

	expr(compiler);
	printf("%.*s", compiler->tokens->data[compiler->currentTokenIndex].text.length, compiler->tokens->data[compiler->currentTokenIndex].text.chars);
	ASSERT(compiler->tokens->data[compiler->currentTokenIndex].type == TOKEN_EOF);

	return compiler->textSection;
}

Result expr(Compiler* compiler)
{
	return commaExpr(compiler);
}

Result primaryExpr(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_IDENTIFIER))
		return;
	if (matchConstant(compiler))
		return;
	if (matchToken(compiler, TOKEN_STRING_LITERAL))
		return; // Later concatenate strings;
	if (matchToken(compiler, TOKEN_LEFT_PAREN))
		return groupingExpr(compiler);
}

bool matchConstant(Compiler* compiler)
{
	return matchToken(compiler, TOKEN_CHAR_CONSTANT)
		|| matchToken(compiler, TOKEN_INT_CONSTANT)
		|| matchToken(compiler, TOKEN_UNSIGNED_INT_CONSTANT)
		|| matchToken(compiler, TOKEN_LONG_CONSTANT)
		|| matchToken(compiler, TOKEN_UNSIGNED_LONG_CONSTANT)
		|| matchToken(compiler, TOKEN_LONG_LONG_CONSTANT)
		|| matchToken(compiler, TOKEN_UNSIGNED_LONG_LONG_CONSTANT)
		|| matchToken(compiler, TOKEN_FLOAT_CONSTANT)
		|| matchToken(compiler, TOKEN_DOUBLE_CONSTANT)
		|| matchToken(compiler, TOKEN_LONG_DOUBLE_CONSTANT);
}

Result groupingExpr(Compiler* compiler)
{
	Result result = expr(compiler);
	expectToken(compiler, TOKEN_RIGHT_PAREN, "expected ')'");
	return result;
}

Result postfixExpr(Compiler* compiler)
{
	Result result = primaryExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_LEFT_BRACKET))
			result = indexExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_LEFT_PAREN))
			result = callExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_DOT))
			result = memberAccessExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_ARROW))
			result = dereferenceAndMemberAccessExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_PLUS_PLUS))
			result = postIncrementExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_MINUS_MINUS))
			result = postDecrementExpr(compiler, &result);
		else
			break;
	}

	return result;
}

Result indexExpr(Compiler* compiler, const Result* lhs)
{
	Result rhs = expr(compiler);
	expectToken(compiler, TOKEN_RIGHT_BRACKET, "expected ']'");

	/*Result result = compileIndexExpr(compiler, lhs, rhs);
	return result;*/

	return;
}

Result callExpr(Compiler* compiler, const Result* lhs)
{
	if (matchToken(compiler, TOKEN_RIGHT_PAREN))
	{
		return;
	}

	do
	{
		assignmentExpr(compiler);
	} while (matchToken(compiler, TOKEN_COMMA));


	expectToken(compiler, TOKEN_RIGHT_PAREN, "expected ')'");
}

Result memberAccessExpr(Compiler* compiler, const Result* lhs)
{
	expectToken(compiler, TOKEN_IDENTIFIER, "expected member name");

	return;
}

Result dereferenceAndMemberAccessExpr(Compiler* compiler, const Result* lhs)
{
	expectToken(compiler, TOKEN_IDENTIFIER, "expected member name");

	return;
}

Result postIncrementExpr(Compiler* compiler, const Result* lhs)
{
	return;
}

Result postDecrementExpr(Compiler* compiler, const Result* lhs)
{
	return;
}

Result unaryExpr(Compiler* compiler)
{
	Result result; // = RESULT_ERROR;

	if (matchToken(compiler, TOKEN_PLUS_PLUS))
		result = preIncrementExpr(compiler);
	else if (matchToken(compiler, TOKEN_MINUS_MINUS))
		result = preDecrementExpr(compiler);
	else if (matchToken(compiler, TOKEN_AND))
		result = referenceExpr(compiler);
	else if (matchToken(compiler, TOKEN_STAR))
		result = dereferenceExpr(compiler);
	else if (matchToken(compiler, TOKEN_PLUS))
		result = unaryPlusExpr(compiler);
	else if (matchToken(compiler, TOKEN_MINUS))
		result = unaryMinusExpr(compiler);
	else if (matchToken(compiler, TOKEN_TILDE))
		result = bitwiseNotExpr(compiler);
	else if (matchToken(compiler, TOKEN_BANG))
		result = logicalNotExpr(compiler);
	else if (matchToken(compiler, TOKEN_SIZEOF))
		result = sizeofExpr(compiler);
	else
		result = postfixExpr(compiler);

	return result;
}

Result preIncrementExpr(Compiler* compiler)
{
	Result operand = unaryExpr(compiler);
	return;
}

Result preDecrementExpr(Compiler* compiler)
{
	Result operand = unaryExpr(compiler);
	return;
}

Result referenceExpr(Compiler* compiler)
{
	castExpr(compiler);
	return;
}

Result dereferenceExpr(Compiler* compiler)
{
	castExpr(compiler);
	return;
}

Result unaryPlusExpr(Compiler* compiler)
{
	castExpr(compiler);
	return;
}

Result unaryMinusExpr(Compiler* compiler)
{
	castExpr(compiler);
	return;
}

Result bitwiseNotExpr(Compiler* compiler)
{
	castExpr(compiler);
	return;
}

Result logicalNotExpr(Compiler* compiler)
{
	castExpr(compiler);
	return;
}

Result sizeofExpr(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_LEFT_PAREN))
	{
		// if next token is type name sizeof type name do this else groupingExpr(compiler);
		DataType type = typeName(compiler);

		expectToken(compiler, TOKEN_RIGHT_PAREN, "expected ')'");
		return;
	}
	unaryExpr(compiler);
	return;
}

Result castExpr(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_LEFT_PAREN))
	{
		DataType type = typeName(compiler);
		expectToken(compiler, TOKEN_RIGHT_PAREN, "expected ')'");
		castExpr(compiler);
		return;
	}
	else
	{
		return unaryExpr(compiler);
	}
}

Result multiplicativeExpr(Compiler* compiler)
{
	Result result = castExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_STAR))
			result = multiplicationExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_SLASH))
			result = divisionExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_PERCENT))
			result = moduloExpr(compiler, &result);
		else
			break;
	}

	return result;
}

Result multiplicationExpr(Compiler* compiler, const Result* lhs)
{
	Result rhs = castExpr(compiler);
	return;
}

Result divisionExpr(Compiler* compiler, const Result* lhs)
{
	Result rhs = castExpr(compiler);
	return;
}

Result moduloExpr(Compiler* compiler, const Result* lhs)
{
	Result rhs = castExpr(compiler);
	return;
}

Result additiveExpr(Compiler* compiler)
{
	Result result = multiplicativeExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_PLUS))
			result = additionExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_MINUS))
			result = subtractionExpr(compiler, &result);
		else
			break;
	}

	return result;
}

Result additionExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = multiplicativeExpr(compiler);
	return;
}

Result subtractionExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = multiplicativeExpr(compiler);
	return;
}

Result shiftExpr(Compiler* compiler)
{
	Result result = additiveExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_SHIFT_LEFT))
			result = shiftLeftExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_SHIFT_RIGHT))
			result = shiftRightExpr(compiler, &result);
		else
			break;
	}

	return result;
}

Result shiftLeftExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = additiveExpr(compiler);
	return;
}

Result shiftRightExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = additiveExpr(compiler);
	return;
}

Result relationalExpr(Compiler* compiler)
{
	Result result = shiftExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_LESS))
			result = lessThanExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_MORE))
			result = moreThanExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_MORE_EQUALS))
			result = moreThanOrEqualExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_LESS_EQUALS))
			result = lessThanOrEqualExpr(compiler, &result);
		else
			break;
	}

	return result;
}

Result lessThanExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = shiftExpr(compiler);
	return;
}

Result moreThanExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = shiftExpr(compiler);
	return;
}

Result lessThanOrEqualExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = shiftExpr(compiler);
	return;
}

Result moreThanOrEqualExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = shiftExpr(compiler);
	return;
}

Result equalityExpr(Compiler* compiler)
{
	Result result = relationalExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_EQUALS_EQUALS))
			result = equalsExpr(compiler, &result);
		else if (matchToken(compiler, TOKEN_BANG_EQUALS))
			result = notEqualsExpr(compiler, &result);
		else
			break;
	}

	return result;
}

Result equalsExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = relationalExpr(compiler);
	return;
}

Result notEqualsExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = relationalExpr(compiler);
	return;
}

Result bitwiseAndExpr(Compiler* compiler)
{
	Result lhs = equalityExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_AND))
		{
			Result rhs = equalityExpr(compiler);
			//lhs = RESULT_SUCCESS;
		}
		else
		{
			break;
		}
	}

	return lhs;
}

Result bitwiseXorExpr(Compiler* compiler)
{
	Result lhs = bitwiseAndExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_XOR))
		{
			Result rhs = bitwiseAndExpr(compiler);
			//lhs = RESULT_SUCCESS;
		}
		else
		{
			break;
		}
	}

	return lhs;
}

Result bitwiseOrExpr(Compiler* compiler)
{
	Result lhs = bitwiseXorExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_OR))
		{
			Result rhs = bitwiseXorExpr(compiler);
			//lhs = RESULT_SUCCESS;
		}
		else
		{
			break;
		}
	}

	return lhs;
}

Result logicalOrExpr(Compiler* compiler)
{
	Result lhs = bitwiseOrExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_OR_OR))
		{
			Result rhs = bitwiseOrExpr(compiler);
			//lhs = RESULT_SUCCESS;
		}
		else
		{
			break;
		}
	}

	return lhs;
}

Result logicalAndExpr(Compiler* compiler)
{
	Result lhs = logicalOrExpr(compiler);

	while (isCompilerAtEnd(compiler) == false)
	{
		if (matchToken(compiler, TOKEN_AND_AND))
		{
			Result rhs = logicalOrExpr(compiler);
			//lhs = RESULT_SUCCESS;
		}
		else
		{
			break;
		}
	}

	return lhs;
}

Result conditionalExpr(Compiler* compiler)
{
	Result result = logicalOrExpr(compiler);

	if (matchToken(compiler, TOKEN_QUESTION))
	{
		// Move this to compilation later
		Result left = expr(compiler);
		expectToken(compiler, TOKEN_COLON, "expected ':'");
		Result right = conditionalExpr(compiler);
	}

	return result;
}

Result assignmentExpr(Compiler* compiler)
{
	const Result lhs = conditionalExpr(compiler);
	
	if (matchToken(compiler, TOKEN_EQUALS))
		return simpleAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_STAR_EQUALS))
		return multiplyAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_SLASH_EQUALS))
		return divideAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_PERCENT_EQUALS))
		return moduloAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_PLUS_EQUALS))
		return addAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_MINUS_EQUALS))
		return subtractAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_SHIFT_LEFT_EQUALS))
		return shiftLeftAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_SHIFT_RIGHT_EQUALS))
		return shiftRightAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_AND_EQUALS))
		return bitwiseAndAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_XOR_EQUALS))
		return bitwiseXorAssignmentExpr(compiler, &lhs);
	if (matchToken(compiler, TOKEN_OR_EQUALS))
		return bitwiseOrAssignmentExpr(compiler, &lhs);
}

Result simpleAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result multiplyAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result divideAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result moduloAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result addAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result subtractAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result shiftLeftAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result shiftRightAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result bitwiseAndAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result bitwiseXorAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result bitwiseOrAssignmentExpr(Compiler* compiler, const Result* lhs)
{
	const Result rhs = assignmentExpr(compiler);
	return rhs;
}

Result commaExpr(Compiler* compiler)
{
	Result result;

	do
	{
		result = assignmentExpr(compiler);
	} while (matchToken(compiler, TOKEN_COMMA));

	return result;
}

void applyTypeSpecifier(Compiler* compiler, DataType* type)
{
	//if (matchToken(compiler, TOKEN_VOID))
	//	type->type = DATA_TYPE_VOID;
	//else if (matchToken(compiler, TOKEN_CHAR))
	//else if (matchToken(compiler, TOKEN_SHORT))
	//else if (matchToken(compiler, TOKEN_INT))
	//else if (matchToken(compiler, TOKEN_LONG))
	//else if (matchToken(compiler, TOKEN_FLOAT))
	//else if (matchToken(compiler, TOKEN_DOUBLE))
	//else if (matchToken(compiler, TOKEN_))
		
}

void declaration(Compiler* compiler)
{

}

Result declarationSpecifiers(Compiler* compiler)
{
	StorageClass storageClass = STORAGE_CLASS_NONE;

	DataType type;
	type.type = DATA_TYPE_NONE;

	for (;;)
	{
		if (storageClass == STORAGE_CLASS_NONE)
		{
			storageClass = storageClassSpecifier(compiler);
		}
		else
		{
			compilerError(compiler, "multiple storage class specifiers not allowed");
			return;
		}

		//if ()
	}
}

StorageClass storageClassSpecifier(Compiler* compiler)
{
	switch (peekToken(compiler).type)
	{
		case TOKEN_TYPEDEF:  advanceCompiler(compiler); return STORAGE_CLASS_TYPEDEF;
		case TOKEN_EXTERN:   advanceCompiler(compiler); return STORAGE_CLASS_EXTERN;
		case TOKEN_STATIC:   advanceCompiler(compiler); return STORAGE_CLASS_STATIC;
		case TOKEN_AUTO:     advanceCompiler(compiler); return STORAGE_CLASS_AUTO;
		case TOKEN_REGISTER: advanceCompiler(compiler); return STORAGE_CLASS_REGISTER;
	}
}

// An object can be both const and volatile
TypeQualifier typeQualifier(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_CONST))
		return TYPE_QUALIFIER_CONST;
	if (matchToken(compiler, TOKEN_VOLATILE))
		return TYPE_QUALIFIER_VOLATILE;
	else
		return TYPE_QUALIFIER_NONE;
}

DataType typeSpecifier(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_STRUCT))
	{
		return structSpecifier(compiler);
	}
	else if (matchToken(compiler, TOKEN_UNION))
	{
		return unionSpecifier(compiler);
	}
	else if (matchToken(compiler, TOKEN_ENUM))
	{
		return enumSpecifier(compiler);
	}
	else if (matchToken(compiler, TOKEN_IDENTIFIER))
	{
		//return resolveTypedef()
	}
	else
	{

	}

}

DataType structSpecifier(Compiler* compiler)
{
	// resolve struct or parse declarator
	if (matchToken(compiler, TOKEN_IDENTIFIER))
	{

	}
	else if (matchToken(compiler, TOKEN_LEFT_BRACE))
	{

	}

	DataType type;
	type.type = DATA_TYPE_ERROR;
	return type;
}

DataType unionSpecifier(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_IDENTIFIER))
	{

	}
	else if (matchToken(compiler, TOKEN_LEFT_BRACE))
	{

	}

	DataType type;
	type.type = DATA_TYPE_ERROR;
	return type;
}

DataType enumSpecifier(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_IDENTIFIER))
	{

	}
	else if (matchToken(compiler, TOKEN_LEFT_BRACE))
	{

	}

	DataType type;
	type.type = DATA_TYPE_ERROR;
	return type;
}

DataType fundamentalTypeSpecifier(Compiler* compiler)
{
	DataType type;
	type.type = DATA_TYPE_NONE;
	type.isUnsigned = false;

	bool isSignednessSpecified = false;

	if (matchToken(compiler, TOKEN_UNSIGNED))
	{
		isSignednessSpecified = true;
		type.isUnsigned = true;
	}
	else
	{
		if (matchToken(compiler, TOKEN_SIGNED))
			isSignednessSpecified = true;
		type.isUnsigned = false;
	}

	if (matchToken(compiler, TOKEN_VOID))
	{
		type.type = DATA_TYPE_VOID;
	}
	else if (matchToken(compiler, TOKEN_CHAR))
	{
		type.type = DATA_TYPE_CHAR;
	}
	else if (matchToken(compiler, TOKEN_SHORT))
	{
		matchToken(compiler, TOKEN_INT);
		type.type = DATA_TYPE_SHORT;
	}
	else if (matchToken(compiler, TOKEN_INT))
	{
		type.type = DATA_TYPE_INT;
	}
	else if (matchToken(compiler, TOKEN_LONG))
	{
		if (matchToken(compiler, TOKEN_LONG))
		{
			matchToken(compiler, TOKEN_INT);
			type.type = DATA_TYPE_LONG_LONG;
		}
		else if (matchToken(compiler, TOKEN_DOUBLE))
		{
			type.type = DATA_TYPE_LONG_DOUBLE;
		}
		else
		{
			matchToken(compiler, TOKEN_INT);
			type.type = DATA_TYPE_LONG;
		}
	}
	else if (matchToken(compiler, TOKEN_FLOAT))
	{
		if (isSignednessSpecified)
		{
			compilerError(compiler, "cannot use both float and a signedness specifier in type specifier");
			type.type = DATA_TYPE_ERROR;
		}
		else
		{
			type.type = DATA_TYPE_FLOAT;
		}
	}
	else if (matchToken(compiler, TOKEN_DOUBLE))
	{
		if (isSignednessSpecified)
		{
			compilerError(compiler, "cannot use both float and a signedness specifier in type specifier");
			type.type = DATA_TYPE_ERROR;
		}
		else
		{
			type.type = DATA_TYPE_DOUBLE;
		}
	}

	return;
}

DataType typeName(Compiler* compiler)
{
	return;
}

void stmt(Compiler* compiler)
{
	if (checkToken(compiler, TOKEN_IDENTIFIER) && checkNextToken(compiler, TOKEN_COLON))
		labeledStmt(compiler);
	else if (matchToken(compiler, TOKEN_GOTO))
		gotoStatement(compiler);
	else if (matchToken(compiler, TOKEN_BREAK))
		breakStatement(compiler);
	else if (matchToken(compiler, TOKEN_CONTINUE))
		continueStatement(compiler);
	else if (matchToken(compiler, TOKEN_RETURN))
		returnStatement(compiler);
}

void exprStmt(Compiler* compiler)
{
	if (matchToken(compiler, TOKEN_SEMICOLON))
		return;

	expr(compiler);
	expectToken(compiler, TOKEN_SEMICOLON, "expected ';'");
}

void labeledStmt(Compiler* compiler)
{
	Token name = peekToken(compiler);
	advanceCompiler(compiler);
	advanceCompiler(compiler);

	stmt(compiler);
}

void gotoStatement(Compiler* compiler)
{

}

void breakStatement(Compiler* compiler)
{

}

void continueStatement(Compiler* compiler)
{

}

void returnStatement(Compiler* compiler)
{

}


void compilerErrorAtVa(Compiler* compiler, Token token, const char* format, va_list args)
{
	const char* filename = compiler->fileInfo->filename;
	int line = token.line;

	fprintf(
		stderr,
		"%s:%zu:%zu: " TERM_COL_RED "error: " TERM_COL_RESET,
		filename, line + 1, 0
	);

	vfprintf(stderr, format, args);

	fprintf(stderr, "\n");
}

void compilerErrorAt(Compiler* compiler, Token token, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	compilerErrorAtVa(compiler, token, format, args);
	va_end(args);
}

void compilerError(Compiler* compiler, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	compilerErrorAtVa(compiler, peekToken(compiler), format, args);
	va_end(args);
}

void advanceCompiler(Compiler* compiler)
{
	printf("%.*s ", compiler->tokens->data[compiler->currentTokenIndex].text.length, compiler->tokens->data[compiler->currentTokenIndex].text.chars);
	if (isCompilerAtEnd(compiler) == false)
		compiler->currentTokenIndex++;
}

Token peekToken(Compiler* compiler)
{
	return compiler->tokens->data[compiler->currentTokenIndex];
}

Token peekNextToken(Compiler* compiler)
{
	if (isCompilerAtEnd(compiler))
		return compiler->tokens->data[compiler->currentTokenIndex];
	return compiler->tokens->data[compiler->currentTokenIndex + 1];
}

bool expectToken(Compiler* compiler, TokenType type, const char* errorMessage)
{
	if (matchToken(compiler, type))
		return true;
	compilerError(compiler, errorMessage);
	return false;
}

bool isCompilerAtEnd(Compiler* compiler)
{
	return compiler->tokens->data[compiler->currentTokenIndex].type == TOKEN_EOF;
}

bool checkToken(Compiler* compiler, TokenType type)
{
	return peekToken(compiler).type == type;
}

bool checkNextToken(Compiler* compiler, TokenType type)
{
	return peekNextToken(compiler).type == type;
}

bool matchToken(Compiler* compiler, TokenType type)
{
	if (checkToken(compiler, type))
	{
		advanceCompiler(compiler);
		return true;
	}
	return false;
}
