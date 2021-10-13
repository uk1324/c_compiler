
#include "Compiler.h"

// Later add function for the parser, compiler and scanner to reset so they can compile multiple files.

// Use more pushes

// Place function delcarations on top of c files to show there interface

// Add source to FileInfo and a function like StringView FileInfoGetLine(int line);

// It would be nice to replace the switch statements with a vtable
// but this would need to allow the ast to call private compier functions
// it would also be harder to add new functionality like pre compilation type checking or printing

// Make compiler function taking Expr and Stmt const

// Could use restrict pointers in copy functions

// Could remove the compile in functions in compiler
// Later I will use the visitor pattern so it doesn't matter that much

// Could simplify and make long and int the same type

// emitInstructionTempOprand
// emitInstructionOperandTemp
// or maybe move
// the temp takes the size of the operand

// Result location temp baseOffset and size

// Could do basic dead code elimination by checking if an expression has side effect
// and calling endScope on return

// Make beginScope function take a Scope* so it remains on the stack

// Add ast node error

int main(int argCount, char* args[])
{
	const char* filename = "src/triangle.txt";

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

	String output = CompilerCompile(&compiler, &fileInfo, &ast);
	if (compiler.hadError)
		return EXIT_FAILURE;

	printf("%s", output.chars);

	StringFree(&source);
	ParserFree(&parser);
	FileInfoFree(&fileInfo);
	CompilerFree(&compiler);
	for (size_t i = 0; i < ast.size; i++)
	{
		StmtFree(ast.data[i]);
	}
	StmtArrayFree(&ast);

	return EXIT_SUCCESS;
}