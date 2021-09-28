
#include "Compiler.h"

// Later add function for the parser, compiler and scanner to reset so they can compile multiple files.

// Maybe add empty or clear function to array

int main(int argCount, char* args[])
{
	const char* filename = "src/test.txt";
	String source = StringFromFile(filename);

	Scanner scanner;
	ScannerInit(&scanner, filename, source.chars, source.length);

	Parser parser;
	ParserInit(&parser, &scanner);
	StmtArray ast = ParserParse(&parser);
	if (parser.hadError)
		return 1;

	Compiler compiler;
	CompilerInit(&compiler);
	CompilerCompile(&compiler, &parser, &ast);
	

	StringFree(&source);
	ScannerFree(&scanner);
	ParserFree(&parser);

	for (size_t i = 0; i < ast.size; i++)
	{
		StmtFree(ast.data[i]);
	}
}