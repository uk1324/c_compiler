#include <stdio.h>
#include <stdlib.h>

#include "Scanner.h"
#include "String.h"
#include "Parser.h"
#include "Compiler.h"

#include "Ast.h"

#include "AstPrinter.h"

#include "Table.h"

#include "Variable.h"
#include "Json.h"

int main(int argc, char* argv)
{
	//VariableTable table;
	//TableInit(&table);

	//srand(0);

	//for (int i = 0; i < 32; i++)
	//{
	//	char str[10];
	//	for (size_t j = 0; j < 10; j++)
	//		str[j] = rand() % 10 + 'a';
	//	str[8] = '\0';

	//	String key = StringNonOwning(str);
	//	Variable value;
	//	value.baseOffset = 2;
	//	value.isUnsigned = false;

	//	TableSet(&table, &key, value);
	//	
	//	for (size_t i = 0; i < table.capacity; i++)
	//	{
	//		if (table.data[i].key.chars != NULL)
	//		{
	//			VariableTableEntry* entry = &table.data[i];
	//			printf("%s %d, %d\n", entry->key.chars, entry->value.baseOffset, entry->key.length);
	//			while (entry->next != NULL)
	//			{
	//				printf("%s %d, %d\n", entry->key.chars, entry->value.baseOffset, entry->key.length);
	//				entry = entry->next;
	//			}
	//		}
	//	}

	//	printf("capacity: %d size: %d\n", table.capacity, table.size);
	//}

	//TableFree(&table);

	const char* filename = "src/test.txt";
	String source = StringFromFile(filename);
	Scanner scanner;
	ScannerInit(&scanner, filename, source.chars, source.length);
	Parser parser;
	ParserInit(&parser, &scanner);
	StmtArray ast = ParserParse(&parser);
	if (parser.hadError) return 1;

	Compiler compiler;
	//CompilerInit(&compiler);
	CompilerCompile(&compiler, &parser, &ast);
	

	StringFree(&source);
	ScannerFree(&scanner);
	ParserFree(&parser);

	for (size_t i = 0; i < ast.size; i++)
	{
		StmtFree(ast.data[i]);
	}

	//VariableTable table;
	//TableInit(&table);

	//srand(0);

	//for (int i = 0; i < 32; i++)
	//{
	//	char str[10];
	//	for (size_t j = 0; j < 10; j++)
	//		str[j] = rand() % 10 + 'a';
	//	str[8] = '\0';

	//	String key = StringNonOwning(str);
	//	Variable value;
	//	value.baseOffset = 2;
	//	value.isUnsigned = false;

	//	TableSet(&table, &key, value);
	//	
	//	for (size_t i = 0; i < table.capacity; i++)
	//	{
	//		if (table.data[i].key.chars != NULL)
	//		{
	//			VariableTableEntry* entry = &table.data[i];
	//			printf("%s %d, %d\n", entry->key.chars, entry->value.baseOffset, entry->key.length);
	//			while (entry->next != NULL)
	//			{
	//				printf("%s %d, %d\n", entry->key.chars, entry->value.baseOffset, entry->key.length);
	//				entry = entry->next;
	//			}
	//		}
	//	}

	//	printf("capacity: %d size: %d\n", table.capacity, table.size);
	//}

	//TableFree(&table);

	//while (true)
	//{
	//	VariableTable table;
	//	TableInit(&table);
	//	String key = StringNonOwning("a");
	//	Variable value;
	//	value.baseOffset = 2;
	//	value.isUnsigned = true;
	//	value.type = VARIABLE_INT;
	//	TableSet(&table, &key, value);
	//	Variable variable;
	//	TableGet(&table, &key, &variable);
	//	//TableRemove(&table, &key);
	//	printf("%d %d %d\n", value.baseOffset, value.isUnsigned, value.type);
	//	TableFree(&table);
	//}

	//const char* filename = "src/test.txt";
	//String text = StringFromFile(filename);
	//Scanner scanner;
	//ScannerInit(&scanner, filename, text.chars, text.length);
	//Token token;

	//do
	//{
	//	token = ScannerNextToken(&scanner);
	//	printf("%.*s %s\n", token.length, token.chars, TokenTypeToString(token.type));
	//} while (token.type != TOKEN_EOF);



	//while (true)
	//{
		//const char* aa = "a";
		//const char** a = &aa;
		//IntTable table;
		//TableInit(&table);
		//TableSet(&table, a, 6);
		////TableRemove(&table, a);
		//int value;
		//printf("%d", TableGet(&table, a, &value));
		////TableSet(&table, "b", 16);
		////int result;
		////TableGet(&table, "b", &result);
		////TableGet(&table, "f", &result);
		//TableFree(&table);
	//}

	//Table a;
	//TableInit(&a);
	//TableSet(&a, "abc", 2);
	//TableSet(&a, "abc", 3);
	//TableSet(&a, "e", 10);
	//valueType val;
	//TableGet(&a, "abc", &val);
	//printf("%d\n", val);
	//TableGet(&a, "e", &val);
	//printf("%d\n", val);

	//String str = StringCopy("abc def ghi");
	//StringAppendFormat(&str, "hello %d %d", 16, 23);
	//puts(str.chars);
	//StringFree(&str);
	//const char* filename = "src/test.txt";
	//String text = StringFromFile(filename);
	//Scanner scanner;
	////printf("%d\n", text.length);
	//ScannerInit(&scanner, filename, text.chars, text.length);
	//
	//Parser parser;
	//ParserInit(&parser, &scanner);
	//Expr* ast = ParserParse(&parser);
	//printExpr(ast, 0);
	//
	////do
	////{
	////	token = ScannerNextToken(&scanner);
	////	printf("%.*s %d\n", scanner.current.length, scanner.current.chars, scanner.current.type);
	////} while (token.type != TOKEN_EOF);
	//ScannerFree(&scanner);
	//StringFree(&text);
}