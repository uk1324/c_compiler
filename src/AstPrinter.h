// Might later store data like indentation in this
// Could also just put in in the function call
#pragma once

#include "Ast.h"
#include "Assert.h"

const char* TokenTypeToString(TokenType type);

void printExpr(Expr* expr, int depth);
void printStmt(Stmt* statement, int depth);