#include "Ast.h"
#include "Assert.h"
#include "Generic.h"

Expr* ExprAllocate(size_t size, ExprType type)
{
	Expr* expr = malloc(size);
	if (expr == NULL)
	{
		fputs("Failed to allocate Expr", stderr);
		exit(1);
	}
	expr->type = type;
	return expr;
}

void ExprFree(Expr* expression)
{
	switch (expression->type)
	{
		case EXPR_BINARY:
		{
			ExprBinary* expr = (ExprBinary*)expression;
			ExprFree(expr->left);
			ExprFree(expr->right);
			break;
		}
			
		case EXPR_UNARY:
		{
			ExprUnary* expr = (ExprUnary*)expression;
			ExprFree(expr->operand);
			break;
		}

		case EXPR_GROUPING:
			ExprFree(((ExprGrouping*)expression)->expression);
			break;

		case EXPR_NUMBER_LITERAL:
			break;
			
		case EXPR_IDENTIFIER:
			break;

		case EXPR_ASSIGNMENT:
		{
			ExprAssignment* expr = (ExprAssignment*)expression;
			ExprFree(expr->left);
			ExprFree(expr->right);
			break;
		}

		default:
			ASSERT_NOT_REACHED();
			break;
	}
	free(expression);
}

static void copyStmt(Stmt** dst, Stmt** src)
{
	*dst = *src;
}

ARRAY_TEMPLATE_DEFINITION(StmtArray, Stmt*, copyStmt, NO_OP_FUNCTION)

Stmt* StmtAllocate(size_t size, StmtType type)
{
	Stmt* stmt = malloc(size);
	if (stmt == NULL)
	{
		fputs("Failed to allocate Stmt", stderr);
		exit(1);
	}
	stmt->type = type;
	return stmt;
}

void StmtFree(Stmt* statement)
{
	switch (statement->type)
	{
		case STMT_EXPRESSION:
		{
			StmtExpression* stmt = (StmtExpression*)statement;
			ExprFree(stmt->expresssion);
			break;
		}

		case STMT_VARIABLE_DECLARATION:
		{
			StmtVariableDeclaration* stmt = (StmtVariableDeclaration*)statement;
			if (stmt->initializer != NULL)
				ExprFree(stmt->initializer);
			break;
		}

		case STMT_RETURN:
		{
			StmtReturn* stmt = (StmtReturn*)statement;
			ExprFree(stmt->returnValue);
			break;
		}

		case STMT_BLOCK:
		{
			StmtBlock* stmt = (StmtBlock*)statement;
			for (size_t i = 0; i < stmt->satements.size; i++)
			{
				StmtFree(stmt->satements.data[i]);
			}
			break;
		}

		case STMT_IF:
		{
			StmtIf* stmt = (StmtIf*)statement;
			StmtFree(stmt->thenBlock);
			if (stmt->elseBlock != NULL)
				StmtFree(stmt->elseBlock);
			ExprFree(stmt->condition);
			break;
		}

		case STMT_WHILE_LOOP:
		{
			StmtWhileLoop* stmt = (StmtWhileLoop*)statement;
			ExprFree(stmt->condition);
			StmtFree(stmt->body);
			break;
		}

		case STMT_BREAK:
			break;

		case STMT_CONTINUE:
			break;

		case STMT_PUTCHAR:
		{
			StmtPutchar* stmt = (StmtPutchar*)statement;
			ExprFree(stmt->expresssion);
			break;
		}

		default:
			ASSERT_NOT_REACHED();
	}

	free(statement);
}