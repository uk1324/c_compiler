#pragma once

#include <stddef.h>

#include "IntArray.h"
#include "StringView.h"

typedef enum
{
	TOKEN_NUMBER,

	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_ASTERISK,
	TOKEN_SLASH,
	TOKEN_PERCENT,
	TOKEN_LESS_THAN,
	TOKEN_LESS_THAN_EQUALS,
	TOKEN_LESS_THAN_LESS_THAN,
	TOKEN_MORE_THAN,
	TOKEN_MORE_THAN_EQUALS,
	TOKEN_MORE_THAN_MORE_THAN,
	TOKEN_TILDE,
	TOKEN_PIPE,
	TOKEN_PIPE_PIPE,


	TOKEN_SEMICOLON,
	TOKEN_EQUALS,

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
	StringView text;
	int line;
} Token;

typedef struct
{
	char* filename;
	IntArray lineStartOffsets;
} FileInfo;

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

void ScannerInit(Scanner* scanner, const char* filename, const char* text, size_t length);
void ScannerReset(Scanner* scanner, const char* filename, const char* text, size_t length);
void ScannerFree(Scanner* scanner);

Token ScannerNextToken(Scanner* scanner);