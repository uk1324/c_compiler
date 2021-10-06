#include "Scanner.h"
#include "Assert.h"
#include "TerminalColors.h"

#include <stdio.h>
#include <stdbool.h>

void FileInfoInit(FileInfo* fileInfo)
{
	IntArrayInit(&fileInfo->lineStartOffsets);
}

// Don't know if it should remove the newline character
StringView FileInfoGetLine(const FileInfo* fileInfo, size_t lineNumber)
{
	ASSERT((lineNumber) < fileInfo->lineStartOffsets.size);

	StringView line;
	const IntArray* lineStartOffsets = &fileInfo->lineStartOffsets;
	const char* sourceEnd = fileInfo->source.chars + fileInfo->source.length;
	line.chars = fileInfo->source.chars + lineStartOffsets->data[lineNumber];

	line.length = (lineNumber == lineStartOffsets->size)
		? sourceEnd - (fileInfo->source.chars - lineStartOffsets->data[lineNumber])
		: lineStartOffsets->data[lineNumber + 1] - lineStartOffsets->data[lineNumber];
	
	return line;
}

void FileInfoFree(FileInfo* fileInfo)
{
	IntArrayFree(&fileInfo->lineStartOffsets);
}

void ScannerReset(Scanner* scanner, FileInfo* fileInfo)
{
	scanner->line = 0;
	scanner->charInLine = 0;

	scanner->fileInfo = fileInfo;
	IntArrayClear(&scanner->fileInfo->lineStartOffsets);
	IntArrayAppend(&scanner->fileInfo->lineStartOffsets, 0);

	scanner->dataStart = fileInfo->source.chars;
	scanner->dataEnd = fileInfo->source.chars + fileInfo->source.length;
	scanner->currentChar = fileInfo->source.chars;
	scanner->tokenStart = fileInfo->source.chars;
}

void ScannerInit(Scanner* scanner)
{

}

void ScannerFree(Scanner* scanner)
{

}

static void error(Scanner* scanner, const char* message)
{
	const char* chr = scanner->currentChar;
	while ((chr < scanner->dataEnd) && (*chr != '\n'))
		chr++;
	int endOfLineDistance = chr - scanner->currentChar;

	// Make this code better
	// Prints the location of the error and the message then the line the error was in
	// and an arrow below the character the error occurred at
	fprintf(
		stderr,
		"%s:%d:%d: " TERM_COL_RED "error: " TERM_COL_RESET "%s"
		"\n%.*s\n"
		"%*s" TERM_COL_GREEN "^" TERM_COL_RESET "\n",
		scanner->fileInfo->filename, scanner->line, scanner->charInLine, message,
		scanner->charInLine + endOfLineDistance,
		&scanner->dataStart[scanner->fileInfo->lineStartOffsets.data[scanner->line]],
		scanner->charInLine - 1, " "
	);
}

static Token makeToken(Scanner* scanner, TokenType type)
{
	Token token;
	token.text.chars = scanner->tokenStart;
	token.text.length = scanner->currentChar - scanner->tokenStart;
	token.type = type;
	token.line = scanner->line;
	scanner->tokenStart = scanner->currentChar;
	return token;
}

static Token errorToken()
{
	Token token;
	token.text.chars = "";
	token.text.length = 0;
	token.type = TOKEN_ERROR;
	return token;
}

static void advance(Scanner* scanner)
{
	scanner->currentChar++;
	scanner->charInLine++;
}

static void advanceLine(Scanner* scanner)
{
	scanner->charInLine = 0;
	scanner->line++;
	IntArrayAppend(&scanner->fileInfo->lineStartOffsets, scanner->currentChar - scanner->dataStart);
}

static bool isAtEnd(Scanner* scanner)
{
	return scanner->currentChar >= scanner->dataEnd;
}

static char peek(Scanner* scanner)
{
	// Maybe bring this back later
	//ASSERT(isAtEnd(scanner) == false);
	return *scanner->currentChar;
}

static char peekPrevious(Scanner* scanner)
{
	return scanner->currentChar[-1];
}

static bool match(Scanner* scanner, char chr)
{
	if (peek(scanner) == chr)
	{
		advance(scanner);
		return true;
	}
	return false;
}

static char peekNext(Scanner* scanner)
{
	return scanner->currentChar[1];
}

static void skipWhitespace(Scanner* scanner)
{
	for (;;)
	{
		char chr = peek(scanner);

		switch (chr)
		{
			case ' ':
			case '\t':
			case '\r':
				advance(scanner);
				break;

			case '\n':
				advance(scanner);
				advanceLine(scanner);
				break;

			case '/':
				advance(scanner);
				if ((isAtEnd(scanner) == false) && (peek(scanner) == '/'))
				{
					advance(scanner);
					while (isAtEnd(scanner) == false)
					{
						if (peek(scanner) == '\n')
						{
							advanceLine(scanner);
							break;
						}
						advance(scanner);
					}
				}
				else if (peek(scanner) == '*')
				{
					while (isAtEnd(scanner) == false)
					{
						if (match(scanner, '\n'))
						{
							advanceLine(scanner);
						}
						else if (match(scanner, '*'))
						{
							if ((isAtEnd(scanner) == false) && (match(scanner, '/')))
							{
								break;
							}
						}

						advance(scanner);
					}
				}
				break;

			default:
				scanner->tokenStart = scanner->currentChar;
				return;
		}
	}
}

static bool isAlpha(char chr)
{
	return ((chr >= 'A') && (chr <= 'Z'))
		|| ((chr >= 'a') && (chr <= 'z'))
		||  (chr == '_');
}

static bool isDigit(char chr)
{
	return (chr >= '0') && (chr <= '9');
}

static bool isHexDigit(char chr)
{
	return isDigit(chr) || ((chr >= 'a') && (chr <= 'z')) || ((chr >= 'A') && (chr <= 'Z'));
}

static bool isOctalDigit(char chr)
{
	return (chr >= '0') && (chr <= '7');
}

static bool isAlnum(char chr)
{
	return isAlpha(chr) || isDigit(chr);
}

// Validating the number in scanner and then parsing it in the parser is kind of stupid
// but storing the number in token would be cumbersome.
static Token number(Scanner* scanner)
{
	bool isHex = false;

	if (match(scanner, '0'))
	{
		if ((isAtEnd(scanner) == false) && (match(scanner, 'x') || match(scanner, 'X')))
		{
			isHex = true;
			while ((isAtEnd(scanner) == false) && (isHexDigit(peek(scanner))))
				advance(scanner);
		}
		else
		{
			while ((isAtEnd(scanner) == false) && (isOctalDigit(peek(scanner))))
				advance(scanner);
		}
	}
	else
	{
		while ((isAtEnd(scanner) == false) && (isDigit(peek(scanner))))
			advance(scanner);
	}

	bool isFloat = false;

	// Priror to c99 hexiadecimal constant were not allowed put they require special syntax.
	// 0x1.2p3; // hex fraction 1.2 (decimal 1.125) scaled by 2^3, that is 9.0
	// If the float constant starts with a zero the zeros are ignored so
	// there doesn't need to be a special case for octal literals
	if ((isAtEnd(scanner) == false) && match(scanner, '.'))
	{
		if (isHex)
		{
			error(scanner, "hexadecimal floating point constants are not allowed");
			return errorToken();
		}

		isFloat = true;

		while ((isAtEnd(scanner) == false) && (isDigit(peek(scanner))))
			advance(scanner);
	}

	if ((isAtEnd(scanner) == false) && (match(scanner, 'e') || match(scanner, 'E')))
	{
		isFloat = true;

		match(scanner, '-') || match(scanner, '+');

		while ((isAtEnd(scanner) == false) && (isDigit(peek(scanner))))
			advance(scanner);
	}

	if (isFloat && (isAtEnd(scanner) == false))
	{
		if (match(scanner, 'f') || match(scanner, 'F'))
		{
			return makeToken(scanner, TOKEN_FLOAT_LITERAL);
		}
		else if (match(scanner, 'l') || match(scanner, 'L'))
		{
			return makeToken(scanner, TOKEN_LONG_DOUBLE_LITERAL);
		}

		return makeToken(scanner, TOKEN_DOUBLE_LITERAL);
	}
	else
	{
		bool isUnsigned = match(scanner, 'u') || match(scanner, 'U');

		if ((isAtEnd(scanner) == false) && (match(scanner, 'l') || match(scanner, 'L')))
		{
			// The long long literal letters have to have the same case ("LL" or "ll").
			if (peek(scanner) == peekPrevious(scanner))
			{
				return isUnsigned
					? makeToken(scanner, TOKEN_UNSIGNED_LONG_LONG_LITERAL)
					: makeToken(scanner, TOKEN_LONG_LONG_LITERAL);
			}

			return isUnsigned
				? makeToken(scanner, TOKEN_UNSIGNED_LONG_LITERAL)
				: makeToken(scanner, TOKEN_LONG_LITERAL);
		}

		return makeToken(scanner, TOKEN_INT_LITERAL);
	}
}

static Token identifierOrKeyword(Scanner* scanner)
{
	while ((isAtEnd(scanner) == false) && (isAlnum(peek(scanner))))
	{
		advance(scanner);
	}

	TokenType type = TOKEN_IDENTIFIER;

	#define KEYWORD_GROUP(chr) \
		case chr: 

	#define KEYWORD_GROUP_END() \
		break;

	#define KEYWORD(keywordString, tokenType) \
		{ \
			ptrdiff_t keywordLength = sizeof(keywordString) - 1; \
			if (((scanner->currentChar - scanner->tokenStart) == keywordLength) \
			  && (memcmp(scanner->tokenStart, keywordString, keywordLength) == 0)) \
			{ \
				type = tokenType; \
				break; \
			} \
		}

	switch (scanner->tokenStart[0])
	{
		KEYWORD_GROUP('d')
			KEYWORD("double", TOKEN_DOUBLE)
			KEYWORD("do", TOKEN_DO)
		KEYWORD_GROUP_END()

		KEYWORD_GROUP('i')
			KEYWORD("if", TOKEN_IF)
			KEYWORD("int", TOKEN_INT)
		KEYWORD_GROUP_END()

		KEYWORD_GROUP('f')
			KEYWORD("float", TOKEN_FLOAT)
		KEYWORD_GROUP_END()

		KEYWORD_GROUP('l')
			KEYWORD("long", TOKEN_LONG)
		KEYWORD_GROUP_END()

		KEYWORD_GROUP('s')
			KEYWORD("signed", TOKEN_SIGNED)
			KEYWORD("short", TOKEN_SHORT)
		KEYWORD_GROUP_END()

		KEYWORD_GROUP('u')
			KEYWORD("unsigned", TOKEN_UNSIGNED);
		KEYWORD_GROUP_END()

		KEYWORD_GROUP('r')
			KEYWORD("return", TOKEN_RETURN)
		KEYWORD_GROUP_END()

	}

	#undef KEYWORD
	#undef KEYWORD_GROUP
	#undef KEYWORD_GROUP_END

	return makeToken(scanner, type);
}

static Token scanToken(Scanner* scanner)
{
	if (isAtEnd(scanner))
	{
		return makeToken(scanner, TOKEN_EOF);
	}

	skipWhitespace(scanner);
	
	if (isAtEnd(scanner))
	{
		return makeToken(scanner, TOKEN_EOF);
	}

	char chr = peek(scanner);

	if (isDigit(chr))
	{
		return number(scanner);
	}

	// alnum and need to advane char
	if (isAlpha(chr))
	{
		return identifierOrKeyword(scanner);
	}

	if (isAtEnd(scanner))
	{
		return makeToken(scanner, TOKEN_EOF);
	}

	advance(scanner);

	// Maybe use a macro for makeToken
	// Use ternary match for decrement increment
	// Will have to add variants with asigment to many operators
	switch (chr)
	{
		case '+': return makeToken(scanner, TOKEN_PLUS);
		case '-': return makeToken(scanner, TOKEN_MINUS);
		case '*': return makeToken(scanner, TOKEN_ASTERISK);
		case '/': return makeToken(scanner, TOKEN_SLASH);
		case '(': return makeToken(scanner, TOKEN_LEFT_PAREN);
		case ')': return makeToken(scanner, TOKEN_RIGHT_PAREN);
		case ';': return makeToken(scanner, TOKEN_SEMICOLON);
		case '=': return makeToken(scanner, TOKEN_EQUALS);
		case '~': return makeToken(scanner, TOKEN_TILDE);
		case '&': return makeToken(scanner, TOKEN_AMPERSAND);
		case '^': return makeToken(scanner, TOKEN_CIRCUMFLEX);

		case '|': 
		{
			if (match(scanner, '|'))
			{
				return makeToken(scanner, TOKEN_PIPE_PIPE);
			}
			return makeToken(scanner, TOKEN_PIPE);
		}

		case '<':
		{
			if (match(scanner, '<'))
			{
				return makeToken(scanner, TOKEN_LESS_THAN_LESS_THAN);
			}
			else if (match(scanner, '='))
			{
				return makeToken(scanner, TOKEN_LESS_THAN_EQUALS);
			}
			return makeToken(scanner, TOKEN_LESS_THAN);
		}
		case '>':
		{
			if (match(scanner, '>'))
			{
				return makeToken(scanner, TOKEN_MORE_THAN_MORE_THAN);
			}
			else if (match(scanner, '='))
			{
				return makeToken(scanner, TOKEN_MORE_THAN_EQUALS);
			}
			return makeToken(scanner, TOKEN_MORE_THAN);
		}

		default:
			error(scanner, "invalid char");
			return errorToken();
	}
}

Token ScannerNextToken(Scanner* scanner)
{
	return scanToken(scanner);
}