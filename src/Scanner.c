#include "Scanner.h"
#include "Assert.h"
#include "TerminalColors.h"

#include <stdio.h>
#include <stdbool.h>

void ScannerInit(Scanner* scanner, const char* filename, const char* text, size_t length)
{
	scanner->line = 1;
	scanner->charInLine = 0;
	IntArrayInit(&scanner->lineStartOffsets);
	IntArrayAppend(&scanner->lineStartOffsets, 0);
	scanner->filename = filename;

	scanner->dataStart = text;
	scanner->dataEnd = text + length;
	scanner->currentChar = text;
	scanner->tokenStart = text;
}

void ScannerFree(Scanner* scanner)
{
	IntArrayFree(&scanner->lineStartOffsets);
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
		scanner->filename, scanner->line, scanner->charInLine, message,
		scanner->charInLine + endOfLineDistance,
		&scanner->dataStart[scanner->lineStartOffsets.data[scanner->line - 1]],
		scanner->charInLine - 1, " "
	);
}

static Token makeToken(Scanner* scanner, TokenType type)
{
	Token token;
	token.chars = scanner->tokenStart;
	token.length = scanner->currentChar - scanner->tokenStart;
	token.type = type;
	token.line = scanner->line;
	scanner->tokenStart = scanner->currentChar;
	return token;
}

static Token errorToken()
{
	Token token;
	token.chars = "";
	token.length = 0;
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
	IntArrayAppend(&scanner->lineStartOffsets, scanner->currentChar - scanner->dataStart);
}

static bool isAtEnd(Scanner* scanner)
{
	return scanner->currentChar >= scanner->dataEnd;
}

static char peek(Scanner* scanner)
{
	return *scanner->currentChar;
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
		|| (chr == '_');
}

static bool isDigit(char chr)
{
	return (chr >= '0') && (chr <= '9');
}

static bool isAlnum(char chr)
{
	return isAlpha(chr) || isDigit(chr);
}

static Token number(Scanner* scanner)
{
	while ((isAtEnd(scanner) == false) && (isDigit(peek(scanner))))
		advance(scanner);
	return makeToken(scanner, TOKEN_NUMBER);
}

static TokenType matchKeyword(Scanner* scanner, size_t offset, size_t restLength, const char* rest, TokenType type)
{
	if (((scanner->tokenStart + offset + restLength) < scanner->dataEnd)
	 && ((scanner->currentChar - scanner->tokenStart == (offset + restLength)))
	 && (memcmp(scanner->tokenStart + offset, rest, restLength) == 0))
	{
		return type;
	}
	return TOKEN_IDENTIFIER;
}

static Token identifierOrKeyword(Scanner* scanner)
{
	while ((isAtEnd(scanner) == false) && (isAlnum(peek(scanner))))
	{
		advance(scanner);
	}

	TokenType type = TOKEN_IDENTIFIER;

	switch (scanner->tokenStart[0])
	{
		case 'i':
		{
			switch (scanner->tokenStart[1])
			{
				case 'f': type = matchKeyword(scanner, 2, 0, "", TOKEN_IF); break;
				case 'n': type = matchKeyword(scanner, 2, 1, "t", TOKEN_INT); break;
			}
			break;
		}

		case 'd':
		{
			switch (scanner->tokenStart[1])
			{
				case 'o':
				{
					type = matchKeyword(scanner, 2, 0, "", TOKEN_DO);

					switch (scanner->tokenStart[2])
					{
						case 'u': type = matchKeyword(scanner, 3, 3, "ble", TOKEN_DOUBLE); break;
					}
					break;
				}
			}
			break;
		}
	}

	return makeToken(scanner, type);
}

static Token scanToken(Scanner* scanner)
{
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
	switch (chr)
	{
		case '+': return makeToken(scanner, TOKEN_PLUS);
		case '-': return makeToken(scanner, TOKEN_MINUS);
		case '*': return makeToken(scanner, TOKEN_ASTERISK);
		case '/': return makeToken(scanner, TOKEN_SLASH);
		case '(': return makeToken(scanner, TOKEN_LEFT_PAREN);
		case ')': return makeToken(scanner, TOKEN_RIGHT_PAREN);
			
		default:
			error(scanner, "invalid char");
			return errorToken();
	}
}

Token ScannerNextToken(Scanner* scanner)
{
	return scanToken(scanner);
}