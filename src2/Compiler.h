#pragma once

#include "Token.h"
#include "FileInfo.h"
#include "DataType.h"
#include "Result.h"

#include <stdbool.h>
#include <stdarg.h>

//typedef int Result;
//
//#define RESULT_ERROR -1
//#define RESULT_SUCCESS 1

typedef struct
{
	const TokenArray* tokens;
	const FileInfo* fileInfo;

	size_t currentTokenIndex;

	String textSection;

	//TypedefArray typedefTable;

	bool hadError;
	bool isSynchronizing;
} Compiler;

Compiler CompilerInit(Compiler* compiler);
void CompilerFree(Compiler* compiler);
String CompilerCompile(Compiler* compiler, const TokenArray* tokens, const FileInfo* fileInfo);

Result expr(Compiler* compiler);
// Maybe put more newline between the different kinds of expression in the c file
Result primaryExpr(Compiler* compiler);
bool matchConstant(Compiler* compiler);
Result groupingExpr(Compiler* compiler);

Result postfixExpr(Compiler* compiler);
Result indexExpr(Compiler* compiler, const Result* lhs);
Result callExpr(Compiler* compiler, const Result* lhs);
Result memberAccessExpr(Compiler* compiler, const Result* lhs);
Result dereferenceAndMemberAccessExpr(Compiler* compiler, const Result* lhs);
Result postIncrementExpr(Compiler* compiler, const Result* lhs);
Result postDecrementExpr(Compiler* compiler, const Result* lhs);

Result unaryExpr(Compiler* compiler);
Result preIncrementExpr(Compiler* compiler);
Result preDecrementExpr(Compiler* compiler);
Result referenceExpr(Compiler* compiler);
Result dereferenceExpr(Compiler* compiler);
Result unaryPlusExpr(Compiler* compiler);
Result unaryMinusExpr(Compiler* compiler);
Result bitwiseNotExpr(Compiler* compiler);
Result logicalNotExpr(Compiler* compiler);
Result sizeofExpr(Compiler* compiler);

Result castExpr(Compiler* compiler);

Result multiplicativeExpr(Compiler* compiler);
Result multiplicationExpr(Compiler* compiler, const Result* lhs);
Result divisionExpr(Compiler* compiler, const Result* lhs);
Result moduloExpr(Compiler* compiler, const Result* lhs);

Result additiveExpr(Compiler* compiler);
Result additionExpr(Compiler* compiler, const Result* lhs);
Result subtractionExpr(Compiler* compiler, const Result* lhs);

Result shiftExpr(Compiler* compiler);
Result shiftLeftExpr(Compiler* compiler, const Result* lhs);
Result shiftRightExpr(Compiler* compiler, const Result* lhs);

Result relationalExpr(Compiler* compiler);
Result lessThanExpr(Compiler* compiler, const Result* lhs);
Result moreThanExpr(Compiler* compiler, const Result* lhs);
Result lessThanOrEqualExpr(Compiler* compiler, const Result* lhs);
Result moreThanOrEqualExpr(Compiler* compiler, const Result* lhs);

Result equalityExpr(Compiler* compiler);
Result equalsExpr(Compiler* compiler, const Result* lhs);
Result notEqualsExpr(Compiler* compiler, const Result* lhs);

Result bitwiseAndExpr(Compiler* compiler);

Result bitwiseXorExpr(Compiler* compiler);

Result bitwiseOrExpr(Compiler* compiler);

Result logicalOrExpr(Compiler* compiler);

Result logicalAndExpr(Compiler* compiler);

Result conditionalExpr(Compiler* compiler);

Result assignmentExpr(Compiler* compiler);
Result simpleAssignmentExpr(Compiler* compiler, const Result* lhs);
Result multiplyAssignmentExpr(Compiler* compiler, const Result* lhs);
Result divideAssignmentExpr(Compiler* compiler, const Result* lhs);
Result moduloAssignmentExpr(Compiler* compiler, const Result* lhs);
Result addAssignmentExpr(Compiler* compiler, const Result* lhs);
Result subtractAssignmentExpr(Compiler* compiler, const Result* lhs);
Result shiftLeftAssignmentExpr(Compiler* compiler, const Result* lhs);
Result shiftRightAssignmentExpr(Compiler* compiler, const Result* lhs);
Result bitwiseAndAssignmentExpr(Compiler* compiler, const Result* lhs);
Result bitwiseXorAssignmentExpr(Compiler* compiler, const Result* lhs);
Result bitwiseOrAssignmentExpr(Compiler* compiler, const Result* lhs);

Result commaExpr(Compiler* compiler);


//void declarationSpecifiers(Compiler* compiler);
//void declarator(Compiler* compiler);
//void storageTypeSpecifier(Compiler* compiler);
//void applyTypeSpecifier(Compiler* compiler, DataType* type);
void declaration(Compiler* compiler);
//DataType declarationSpecifiers(Compiler* compiler);
StorageClass storageClassSpecifier(Compiler* compiler);
TypeQualifier typeQualifier(Compiler* compiler);
DataType typeSpecifier(Compiler* compiler);
DataType structSpecifier(Compiler* compiler);
DataType unionSpecifier(Compiler* compiler);
DataType enumSpecifier(Compiler* compiler);
DataType fundamentalTypeSpecifier(Compiler* compiler);


DataType typeName(Compiler* compiler);

void stmt(Compiler* compiler);
void exprStmt(Compiler* compiler);
void labeledStmt(Compiler* compiler);
void gotoStatement(Compiler* compiler);
void breakStatement(Compiler* compiler);
void continueStatement(Compiler* compiler);
void returnStatement(Compiler* compiler);

void compilerErrorAtVa(Compiler* compiler, Token token, const char* format, va_list args);
void compilerErrorAt(Compiler* compiler, Token token, const char* format, ...);
void compilerError(Compiler* compiler, const char* format, ...);
void advanceCompiler(Compiler* compiler);
Token peekToken(Compiler* compiler);
Token peekNextToken(Compiler* compiler);
bool expectToken(Compiler* compiler, TokenType type, const char* errorMessage);
bool isCompilerAtEnd(Compiler* compiler);
bool checkToken(Compiler* compiler, TokenType type);
bool checkNextToken(Compiler* compiler, TokenType type);
bool matchToken(Compiler* compiler, TokenType type);