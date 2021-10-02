
#include "Compiler.h"

// Later add function for the parser, compiler and scanner to reset so they can compile multiple files.

// Use more pushes

// Place function delcarations on top of c files to show there interface

// Add source to FileInfo and a function like StringView FileInfoGetLine(int line);

// It would be nice to replace the switch statements with a vtable
// but this would need to allow the ast to call private compier functions
// it would also be harder to add new functionality like pre compilation type checking or printing

int main(int argCount, char* args[])
{
	while (true)
	{
		const char* filename = "src/test.txt";

		String source = StringFromFile(filename);
		FileInfo fileInfo;
		FileInfoInit(&fileInfo);
		Parser parser;
		ParserInit(&parser);
		Compiler compiler;
		CompilerInit(&compiler);

		StmtArray ast = ParserParse(&parser, filename, StringViewFromString(&source), &fileInfo);
		if (parser.hadError)
			return EXIT_FAILURE;

		//CompilerCompile(&compiler, &parser, &ast);

		StringFree(&source);
		ParserFree(&parser);
		FileInfoFree(&fileInfo);
		CompilerFree(&compiler);
		for (size_t i = 0; i < ast.size; i++)
		{
			StmtFree(ast.data[i]);
		}
		StmtArrayFree(&ast);

	}
	return EXIT_SUCCESS;
}