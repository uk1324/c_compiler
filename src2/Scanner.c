#include "Scanner.h"
#include "Generic.h"
#include "TerminalColors.h"

Scanner ScannerInit(Scanner* scanner)
{
	return *scanner;
}

void ScannerFree(Scanner* scanner)
{

}

TokenArray ScannerScan(Scanner* scanner, StringView source, const char* filename, FileInfo* fileInfoToFillOut)
{
	fileInfoToFillOut->source = source;
	fileInfoToFillOut->filename = filename;
	SizetArrayClear(&fileInfoToFillOut->lineStartOffsets);
	SizetArrayAppend(&fileInfoToFillOut->lineStartOffsets, 0);
	scanner->fileInfo = fileInfoToFillOut;

	scanner->currentChar = source.chars;
	scanner->dataEnd = source.chars + source.length;

	scanner->line = 0;


	TokenArray tokens;
	TokenArrayInit(&tokens);

	while (isScannerAtEnd(scanner) == false)
	{
		TokenArrayAppend(&tokens, nextToken(scanner));
	}
	TokenArrayAppend(&tokens, makeToken(scanner, TOKEN_EOF));

	return tokens;
}

Token nextToken(Scanner* scanner)
{
	skipWhitespace(scanner);

	if (isScannerAtEnd(scanner))
		return makeToken(scanner, TOKEN_EOF);

	char chr = peekChar(scanner);

	if (isDigit(chr))
		return number(scanner);

	if (isAlpha(chr))
		return identifierOrKeyword(scanner);

	if (chr == '\'')
		return charConstant(scanner);

	if (chr == '"')
		return stringLiteral(scanner);

	return simpleToken(scanner);
}

void skipWhitespace(Scanner* scanner)
{
	for (;;)
	{
		switch (peekChar(scanner))
		{
			case ' ':
			case '\t':
			case '\r':
				advanceScanner(scanner);
				break;

			case '\n':
				advanceScanner(scanner);
				advanceScannerLine(scanner);
				break;

			case '/':
				if (peekNextChar(scanner) == '/')
					singleLineComment(scanner);
				else if (peekNextChar(scanner) == '*')
					mutliLineComment(scanner);
				else
					goto end;
				break;

			default:
				goto end;
		}
	}

	end: scanner->currentTokenStart = scanner->currentChar;
}

void singleLineComment(Scanner* scanner)
{
	// Skip "//"
	advanceScanner(scanner);
	advanceScanner(scanner);

	while (isScannerAtEnd(scanner) == false)
	{
		if (matchChar(scanner, '\n'))
		{
			advanceScannerLine(scanner);
			break;
		}
		advanceScanner(scanner);
	}
}

void mutliLineComment(Scanner* scanner)
{
	// Skip '/*'
	advanceScanner(scanner);
	advanceScanner(scanner);

	while (isScannerAtEnd(scanner) == false)
	{
		if (matchChar(scanner, '\n'))
		{
			advanceScannerLine(scanner);
		}
		else if (matchChar(scanner, '*'))
		{
			if (matchChar(scanner, '/'))
			{
				break;
			}
		}

		advanceScanner(scanner);
	}
}

Token number(Scanner* scanner)
{
	bool isSignificandHex = false;

	bool nonOctalDigitInSiginificandStartingWithZero = false;
	// C allows floating point literals to start with zero so
	// 018   | error
	// 018.0 | correct

	bool isFloat = false;

	if (matchChar(scanner, '0'))
	{
		if (matchChar(scanner, 'x'))
		{
			isSignificandHex = true;
			if (isHexDigit(peekChar(scanner)) == false)
			{
				scannerError(scanner, "number literal can't end with 'x'");
				return errorToken(scanner);
			}

			while (isHexDigit(peekChar(scanner)))
				advanceScanner(scanner);
		}
		else
		{
			char chr = peekChar(scanner);
			while (isDigit(chr))
			{
				if (isOctalDigit(chr) == false)
				{
					nonOctalDigitInSiginificandStartingWithZero = true;
				}
			}
		}
	}
	else
	{
		while (isDigit(peekChar(scanner)))
			advanceScanner(scanner);
	}

	if (matchChar(scanner, '.'))
	{
		isFloat = true;
		while (isDigit(peekChar(scanner)))
			advanceScanner(scanner);
	}

	if (matchChar(scanner, 'e') || matchChar(scanner, 'E'))
	{
		matchChar(scanner, '-') || matchChar(scanner, '+');
		isFloat = true;
		while (isDigit(peekChar(scanner)))
			advanceScanner(scanner);
	}

	if (isFloat)
	{
		if (isSignificandHex)
		{
			scannerError(scanner, "cannot use hexadecimal whole part in floating point constant");
			return errorToken(scanner);
		}

		return floatSuffix(scanner);
	}
	else
	{
		if (nonOctalDigitInSiginificandStartingWithZero)
		{
			scannerError(scanner, "non octal digit in octal constant");
			return errorToken(scanner);
		}

		return intSuffix(scanner);
	}
}

Token floatSuffix(Scanner* scanner)
{
	if (matchChar(scanner, 'l') || matchChar(scanner, 'L'))
	{
		return makeToken(scanner, TOKEN_LONG_DOUBLE_CONSTANT);
	}
	else if (matchChar(scanner, 'f') || matchChar(scanner, 'F'))
	{
		return makeToken(scanner, TOKEN_FLOAT_CONSTANT);
	}
	return makeToken(scanner, TOKEN_DOUBLE_CONSTANT);
}

Token intSuffix(Scanner* scanner)
{
	// Could add error reporing for suffixes like ulu

	bool isUnsigned = false;

	if (matchUnsignedSuffix(scanner))
		isUnsigned = true;

	if (matchChar(scanner, 'l') || matchChar(scanner, 'L'))
	{
		if (peekChar(scanner) == peekPreviousChar(scanner))
		{
			advanceScanner(scanner);

			if (isUnsigned || matchUnsignedSuffix(scanner))
				return makeToken(scanner, TOKEN_UNSIGNED_LONG_LONG_CONSTANT);
			else
				return makeToken(scanner, TOKEN_LONG_LONG_CONSTANT);
		}
		else if (toLower(peekChar(scanner)) == toLower(peekPreviousChar(scanner)))
		{
			advanceScanner(scanner);
			scannerError(scanner, "long long constant requires both letter to be the same case");
			return errorToken(scanner);
		}

		if (isUnsigned || matchUnsignedSuffix(scanner))
			return makeToken(scanner, TOKEN_UNSIGNED_LONG_CONSTANT);
		else
			return makeToken(scanner, TOKEN_LONG_CONSTANT);
	}

	if (isUnsigned)
		return makeToken(scanner, TOKEN_UNSIGNED_INT_CONSTANT);
	else
		return makeToken(scanner, TOKEN_INT_CONSTANT);
}

bool matchUnsignedSuffix(Scanner* scanner)
{
	return (matchChar(scanner, 'u') || matchChar(scanner, 'U'));
}

Token identifierOrKeyword(Scanner* scanner)
{
	while ((isScannerAtEnd(scanner) == false) && (isAlnum(peekChar(scanner))))
	{
		advanceScanner(scanner);
	}

	TokenType type = TOKEN_IDENTIFIER;

#define KEYWORD_GROUP(chr) \
		case chr: 

#define KEYWORD_GROUP_END() \
		break;

#define KEYWORD(keywordString, tokenType) \
		{ \
			ptrdiff_t keywordLength = sizeof(keywordString) - 1; \
			if (((scanner->currentChar - scanner->currentTokenStart) == keywordLength) \
			  && (memcmp(scanner->currentTokenStart, keywordString, keywordLength) == 0)) \
			{ \
				type = tokenType; \
				break; \
			} \
		}

	switch (*scanner->currentTokenStart)
	{
		KEYWORD_GROUP('a')
			KEYWORD("auto", TOKEN_AUTO)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('b')
			KEYWORD("break", TOKEN_BREAK)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('c')
			KEYWORD("case", TOKEN_CASE)
			KEYWORD("char", TOKEN_CHAR)
			KEYWORD("const", TOKEN_CONST)
			KEYWORD("continue", TOKEN_CONTINUE)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('d')
			KEYWORD("default", TOKEN_DEFAULT)
			KEYWORD("do", TOKEN_DO)
			KEYWORD("double", TOKEN_DOUBLE)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('e')
			KEYWORD("else", TOKEN_ELSE)
			KEYWORD("enum", TOKEN_ENUM)
			KEYWORD("extern", TOKEN_EXTERN)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('f')
			KEYWORD("float", TOKEN_FLOAT)
			KEYWORD("for", TOKEN_FOR)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('g')
			KEYWORD("goto", TOKEN_GOTO)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('i')
			KEYWORD("if", TOKEN_IF)
			KEYWORD("int", TOKEN_INT)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('l')
			KEYWORD("long", TOKEN_LONG)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('r')
			KEYWORD("register", TOKEN_REGISTER)
			KEYWORD("return", TOKEN_RETURN)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('s')
			KEYWORD("short", TOKEN_SHORT)
			KEYWORD("signed", TOKEN_SIGNED)
			KEYWORD("sizeof", TOKEN_SIZEOF)
			KEYWORD("static", TOKEN_STATIC)
			KEYWORD("struct", TOKEN_STRUCT)
			KEYWORD("switch", TOKEN_SWITCH)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('t')
			KEYWORD("typedef", TOKEN_TYPEDEF)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('u')
			KEYWORD("union", TOKEN_UNION)
			KEYWORD("unsigned", TOKEN_UNSIGNED)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('v')
			KEYWORD("void", TOKEN_VOID)
			KEYWORD("volatile", TOKEN_VOLATILE)
		KEYWORD_GROUP_END();

		KEYWORD_GROUP('w')
			KEYWORD("while", TOKEN_WHILE)
		KEYWORD_GROUP_END();
	}

	return makeToken(scanner, type);

#undef KEYWORD_GROUP
#undef KEYWORD_GROUP_END
#undef KEYWORD
}

Token charConstant(Scanner* scanner)
{
	if (matchChar(scanner, '\''))
	{
		scannerError(scanner, "empty char constant not allowed");
		return errorToken(scanner);
	}

	for (;;)
	{

		if (isScannerAtEnd(scanner))
		{
			scannerError(scanner, "unterminated char constant");
			return errorToken(scanner);
		}

		if (matchChar(scanner, '\'') && peekPreviousChar(scanner) != '\\')
		{
			break;
		}

		advanceScanner(scanner);
	}

	return makeToken(scanner, TOKEN_CHAR_CONSTANT);
}

Token stringLiteral(Scanner* scanner)
{
	for (;;)
	{
		advanceScanner(scanner);

		if (isScannerAtEnd(scanner))
		{
			scannerError(scanner, "unterminated string literal");
			return errorToken(scanner);
		}

		if (matchChar(scanner, '"') && peekPreviousChar(scanner) != '\\')
		{
			break;
		}
	}

	return makeToken(scanner, TOKEN_STRING_LITERAL);
}

Token simpleToken(Scanner* scanner)
{
	char chr = peekChar(scanner);
	advanceScanner(scanner);

#define MAKE(type) makeToken(scanner, type)
#define MATCH(chr) matchChar(scanner, chr)

	switch (chr)
	{
		case '[': return MAKE(TOKEN_LEFT_BRACKET);
		case ']': return MAKE(TOKEN_RIGHT_BRACKET);
		case '{': return MAKE(TOKEN_LEFT_BRACE);
		case '}': return MAKE(TOKEN_RIGHT_BRACE);
		case '(': return MAKE(TOKEN_LEFT_PAREN);
		case ')': return MAKE(TOKEN_RIGHT_PAREN);
		case ',': return MAKE(TOKEN_COMMA);
		case ':': return MAKE(TOKEN_COLON);
		case '?': return MAKE(TOKEN_QUESTION);
		case '~': return MAKE(TOKEN_TILDE);
		case ';': return MAKE(TOKEN_SEMICOLON);

		case '=': return MATCH('=') ? MAKE(TOKEN_EQUALS_EQUALS)  : MAKE(TOKEN_EQUALS);
		case '!': return MATCH('=') ? MAKE(TOKEN_BANG_EQUALS)    : MAKE(TOKEN_BANG);
		case '^': return MATCH('=') ? MAKE(TOKEN_XOR_EQUALS)     : MAKE(TOKEN_XOR);
		case '*': return MATCH('=') ? MAKE(TOKEN_STAR_EQUALS)    : MAKE(TOKEN_STAR);
		case '%': return MATCH('=') ? MAKE(TOKEN_PERCENT_EQUALS) : MAKE(TOKEN_PERCENT);

		case '+':
		{
			if (MATCH('+'))
				return MAKE(TOKEN_PLUS_PLUS);
			if (MATCH('='))
				return MAKE(TOKEN_PLUS_EQUALS);
			return MAKE(TOKEN_PLUS);
		}

		case '-':
		{
			if (MATCH('-'))
				return MAKE(TOKEN_MINUS);
			if (MATCH('='))
				return MAKE(TOKEN_MINUS_EQUALS);
			if (MATCH('>'))
				return MAKE(TOKEN_ARROW);
			return MAKE(TOKEN_MINUS);
		}

		case '>':
		{
			if (MATCH('='))
			{
				return MAKE(TOKEN_MORE_EQUALS);
			}
			if (MATCH('>'))
			{
				if (MATCH('='))
					return MAKE(TOKEN_SHIFT_RIGHT_EQUALS);
				return MAKE(TOKEN_SHIFT_RIGHT);
			}
			return MAKE(TOKEN_MORE);
		}

		case '<':
		{
			if (MATCH('='))
			{
				return MAKE(TOKEN_LESS_EQUALS);
			}
			if (MATCH('<'))
			{
				if (MATCH('='))
					return MAKE(TOKEN_SHIFT_LEFT_EQUALS);
				return MAKE(TOKEN_SHIFT_LEFT);
			}
			return MAKE(TOKEN_LESS);
		}

		case '.':
		{
			if ((peekChar(scanner) == '.') && (peekNextChar(scanner) == '.'))
			{
				return MAKE(TOKEN_DOT_DOT_DOT);
			}
			return MAKE(TOKEN_DOT);
		}

		case '&':
		{
			if (MATCH('='))
				return MAKE(TOKEN_AND_EQUALS);
			if (MATCH('&'))
				return MAKE(TOKEN_AND_AND);
			return MAKE(TOKEN_AND);
		}

		case '|':
		{
			if (MATCH('='))
				return MAKE(TOKEN_OR_EQUALS);
			if (MATCH('|'))
				return MAKE(TOKEN_OR_OR);
			return MAKE(TOKEN_OR);
		}

		default:
			scannerError(scanner, "invalid character");
			return errorToken(scanner);
	}

#undef MAKE
#undef MATCH
}

void scannerError(Scanner* scanner, const char* message)
{
	const size_t* lineStartOffsets = scanner->fileInfo->lineStartOffsets.data;
	const char* filename = scanner->fileInfo->filename;
	const char* dataStart = scanner->fileInfo->source.chars;
	size_t line = scanner->line;
	const char* lineStart = dataStart + lineStartOffsets[line];
	size_t lineLength = findEndOfLine(dataStart, scanner->dataEnd) - lineStart;
	size_t errorTokenLength = scanner->currentChar - scanner->currentTokenStart;
	int charInLine = scanner->currentTokenStart - lineStart;

	fprintf(
		stderr, 
		"%s:%zu:%zu: " TERM_COL_RED "error: " TERM_COL_RESET "%s\n"
		"%.*s\n"
		"%*s" TERM_COL_GREEN "^",
		filename, line + 1, charInLine, message,
		lineLength, lineStart,
		charInLine, ""
	);

	// -1 becuase '^' points to the first character.
	for (size_t i = 0; i < (errorTokenLength - 1); i++)
		fputc('~', stderr);

	fprintf(stderr, "\n" TERM_COL_RESET);
}

const char* findEndOfLine(const char* data, const char* dataEnd)
{
	while (data < dataEnd)
	{
		if (*data == '\n')
		{
			return data - 1;
		}
		data++;
	}

	return data;
}

Token makeToken(Scanner* scanner, TokenType type)
{
	Token token;
	token.line = scanner->line;
	token.text.chars = scanner->currentTokenStart;
	token.text.length = scanner->currentChar - scanner->currentTokenStart;
	token.type = type;
	scanner->currentTokenStart = scanner->currentChar;
	return token;
}

Token errorToken(Scanner* scanner)
{
	Token token;
	token.type = TOKEN_ERROR;
	token.line = 0;
	token.text = StringViewInit("", 0);
	scanner->currentTokenStart = scanner->currentChar;
	return token;
}

bool isScannerAtEnd(const Scanner* scanner)
{
	return scanner->currentChar >= scanner->dataEnd;
}

char peekChar(Scanner* scanner)
{
	if (isScannerAtEnd(scanner))
		return '\0';
	else
		return *scanner->currentChar;
}

char peekNextChar(Scanner* scanner)
{
	if ((scanner->currentChar + 1) >= scanner->dataEnd)
		return '\0';
	else
		return scanner->currentChar[1];
}

char peekPreviousChar(Scanner* scanner)
{
	return scanner->currentChar[-1];
}

bool matchChar(Scanner* scanner, char chr)
{
	if (peekChar(scanner) == chr)
	{
		advanceScanner(scanner);
		return true;
	}
	else
	{
		return false;
	}
}

void advanceScanner(Scanner* scanner)
{
	scanner->currentChar++;
}

void advanceScannerLine(Scanner* scanner)
{
	scanner->line++;
	SizetArrayAppend(&scanner->fileInfo->lineStartOffsets, scanner->currentChar - scanner->fileInfo->source.chars);
}

bool isAlpha(char chr)
{
	return ((chr >= 'A') && (chr <= 'Z'))
		|| ((chr >= 'a') && (chr <= 'z'))
		||   chr == '_';
}

bool isDigit(char chr)
{
	return ((chr >= '0') && (chr <= '9'));
}

bool isAlnum(char chr)
{
	return isDigit(chr) || isAlpha(chr);
}

bool isHexDigit(char chr)
{
	return isDigit(chr)
		|| ((chr >= 'A') && (chr <= 'F'))
		|| ((chr >= 'a') && (chr <= 'f'));
}

bool isOctalDigit(char chr)
{
	return ((chr >= '0') && (chr <= '7'));
}

char toLower(char chr)
{
	if ((chr >= 'A') && (chr <= 'Z'))
	{
		return chr + ('a' - 'A');
	}
	if ((chr >= 'a') && (chr <= 'z'))
	{
		return chr - ('a' - 'A');
	}
	else
	{
		return chr;
	}
}