#pragma once

#include <stddef.h>

#include "IntArray.h"
#include "StringView.h"

// Line numbers are counted from 0

typedef enum
{
	TOKEN_INT_LITERAL,
	TOKEN_UNSIGNED_INT_LITERAL,
	TOKEN_LONG_LITERAL,
	TOKEN_UNSIGNED_LONG_LITERAL,
	TOKEN_LONG_LONG_LITERAL,
	TOKEN_UNSIGNED_LONG_LONG_LITERAL,
	TOKEN_FLOAT_LITERAL,
	TOKEN_DOUBLE_LITERAL,
	TOKEN_LONG_DOUBLE_LITERAL,

	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_ASTERISK,
	TOKEN_SLASH,
	TOKEN_PERCENT,
	TOKEN_AMPERSAND,
	TOKEN_CIRCUMFLEX,
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

	TOKEN_CHAR,
	TOKEN_SHORT,
	TOKEN_INT,
	TOKEN_LONG,

	TOKEN_FLOAT,
	TOKEN_DOUBLE,

	TOKEN_SIGNED,
	TOKEN_UNSIGNED,

	TOKEN_DO,

	TOKEN_RETURN
}  TokenType;

typedef struct
{
	TokenType type;
	StringView text;
	size_t line;
} Token;

typedef struct
{
	const char* filename;
	StringView source;
	IntArray lineStartOffsets;
} FileInfo;

void FileInfoInit(FileInfo* fileInfo);
// If line doesn't exist should it return a NULL string view
// or should I just assert
StringView FileInfoGetLine(const FileInfo* fileInfo, size_t lineNumber);
void FileInfoFree(FileInfo* fileInfo);

typedef struct
{
	size_t line;
	int charInLine;

	FileInfo* fileInfo;

	const char* dataStart;
	const char* dataEnd;

	const char* tokenStart;
	const char* currentChar;
} Scanner;

void ScannerReset(Scanner* scanner, FileInfo* fileInfo);
void ScannerInit(Scanner* scanner);
void ScannerFree(Scanner* scanner);

Token ScannerNextToken(Scanner* scanner);