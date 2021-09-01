#include <stdio.h>
#include <stdlib.h>

#include "Scanner.h"
#include "String.h"
#include "Parser.h"

#include "Ast.h"

#include "AstPrinter.h"

int main(int argc, char* argv)
{
	String str = StringCopy("abc def ghi");
	StringAppendFormat(&str, "hello %d %d", 16, 23);
	puts(str.chars);
	StringFree(&str);
	//const char* filename = "src/test.txt";
	//String text = StringFromFile(filename);
	//Scanner scanner;
	////printf("%d\n", text.length);
	//ScannerInit(&scanner, filename, text.chars, text.length);
	//
	//Parser parser;
	//ParserInit(&parser, &scanner);
	//Expr* ast = ParserParse(&parser);
	//printAst(ast);
	//
	////do
	////{
	////	token = ScannerNextToken(&scanner);
	////	printf("%.*s %d\n", scanner.current.length, scanner.current.chars, scanner.current.type);
	////} while (token.type != TOKEN_EOF);
	//ScannerFree(&scanner);
	//StringFree(&text);
}