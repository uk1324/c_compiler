#include "Ast.h"
#include "Assert.h"

void* ExprAllocate(size_t size, ExprType type)
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
			free(expr);
			break;
		}
			
		case EXPR_UNARY:
		{
			ExprUnary* expr = (ExprUnary*)expression;
			ExprFree(expr->operand);
			free(expr);
			break;
		}

		case EXPR_INT_LITERAL:
			free(expression);
			break;
			
		default:
			ASSERT_NOT_REACHED();
			break;
	}
}
