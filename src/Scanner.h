#pragma once

#include <stddef.h>

#include "IntArray.h"

typedef enum
{
	TOKEN_NUMBER,

	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_ASTERISK,
	TOKEN_SLASH,
	TOKEN_PERCENT,

	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,

	TOKEN_EOF,
	TOKEN_ERROR,

	TOKEN_IDENTIFIER,

	TOKEN_IF,
	TOKEN_INT,
	TOKEN_DOUBLE,
	TOKEN_DO
}  TokenType;

typedef struct
{
	TokenType type;
	const char* chars;
	size_t length;
	int line;
} Token;

typedef struct
{
	int line;
	int charInLine;
	IntArray lineStartOffsets;
	const char* filename;

	const char* dataStart;
	const char* dataEnd;
	const char* tokenStart;
	const char* currentChar;
} Scanner;

// Maybe add a function ScannerPrintLine
void ScannerInit(Scanner* scanner, const char* filename, const char* text, size_t length);
void ScannerFree(Scanner* scanner);

Token ScannerNextToken(Scanner* scanner);