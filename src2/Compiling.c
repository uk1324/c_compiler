//#include "Compiling.h"
//#include "Compiler.h"
//
//void beginScope(Compiler* compiler, Scope* scope)
//{
//	VariableArrayInit(&scope->variables);
//	if (compiler->currentScope == NULL)
//	{
//		scope->enclosing = NULL;
//		compiler->currentScope = &scope;
//	}
//	else
//	{
//		scope->enclosing = compiler->currentScope;
//		compiler->currentScope = &scope;
//	}
//}
//
//void endScope(Compiler* compiler)
//{
//	VariableArrayFree(&compiler->currentScope->variables);
//	compiler->currentScope = compiler->currentScope->enclosing;
//}