#include "Scanner.h"
#include "Compiler.h"

int main(int argCount, char* args[])
{
	

	const char* filename = "src2/test.txt";

	FileInfo fileInfo = FileInfoInit(&fileInfo);
	String source = StringFromFile(filename);
	Scanner scanner = ScannerInit(&scanner);
	Compiler compiler = CompilerInit(&compiler);

	TokenArray tokens = ScannerScan(&scanner, StringViewFromString(&source), filename, &fileInfo);
	//for (size_t i = 0; i < tokens.size; i++)
	//{
	//	tokens.data[i].type;
	//	printf("%.*s", tokens.data[i].text.length, tokens.data[i].text.chars);
	//}
	//printf("\n");
	String output = CompilerCompile(&compiler, &tokens, &fileInfo);
	//printf("%.*s", output.length, output.chars);

	FileInfoFree(&fileInfo);
	ScannerFree(&scanner);
	StringFree(&source);
	TokenArrayFree(&tokens);
	CompilerFree(&compiler);
	StringFree(&output);

}