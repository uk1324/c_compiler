#pragma once

#include "StringView.h"
#include "FileInfo.h"
#include "Token.h"

#include <stdbool.h>

typedef struct
{
	size_t line;

	FileInfo* fileInfo;

	const char* dataEnd;

	TokenArray* tokens;

	const char* currentTokenStart;
	const char* currentChar;
} Scanner;

Scanner ScannerInit(Scanner* scanner);
void ScannerFree(Scanner* scanner);
TokenArray ScannerScan(Scanner* scanner, StringView source, const char* filename, FileInfo* fileInfoToFillOut);

Token nextToken(Scanner* scanner);
void skipWhitespace(Scanner* scanner);
// Comments are removed by the preprocessor, but this might be useful later.
void singleLineComment(Scanner* scanner);
void mutliLineComment(Scanner* scanner);
Token number(Scanner* scanner);
Token floatSuffix(Scanner* scanner);
Token intSuffix(Scanner* scanner);
bool matchUnsignedSuffix(Scanner* scanner);
Token identifierOrKeyword(Scanner* scanner);
Token charConstant(Scanner* scanner);
Token stringLiteral(Scanner* scanner);
Token simpleToken(Scanner* scanner);

void scannerError(Scanner* scanner, const char* message);
const char* findEndOfLine(const char* data, const char* dataEnd);

Token makeToken(Scanner* scanner, TokenType type);
Token errorToken(Scanner* scanner);
bool isScannerAtEnd(const Scanner* scanner);
char peekChar(Scanner* scanner);
char peekNextChar(Scanner* scanner);
char peekPreviousChar(Scanner* scanner);
bool matchChar(Scanner* scanner, char chr);
void advanceScanner(Scanner* scanner);
void advanceScannerLine(Scanner* scanner);

bool isAlpha(char chr);
bool isDigit(char chr);
bool isAlnum(char chr);
bool isHexDigit(char chr);
bool isOctalDigit(char chr);
char toLower(char chr);