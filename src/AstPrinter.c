#include "AstPrinter.h"
#include "Assert.h"

static void printTokenType(TokenType type)
{
	//printf("\ntype: %d\n", type);
	switch (type)
	{
		case TOKEN_PLUS:     printf(" + "); break;
		case TOKEN_MINUS:    printf(" - "); break;
		case TOKEN_ASTERISK: printf(" * "); break;
		case TOKEN_SLASH:    printf(" / "); break;
		case TOKEN_PERCENT:  printf(" %% "); break;

		default:
			ASSERT_NOT_REACHED();
	}
}

void printAst(Expr* expression)
{
	switch (expression->type)
	{
		case EXPR_BINARY:
		{
			ExprBinary* expr = (ExprBinary*)expression;
			printf("(");
			printAst(expr->left);
			printTokenType(expr->operator);
			printAst(expr->right);
			printf(")");
			break;
		}

		case EXPR_INT_LITERAL:
		{
			ExprIntLiteral* expr = (ExprIntLiteral*)expression;
			printf("%.*s", expr->literal.length, expr->literal.chars);
			break;
		}

		case EXPR_UNARY:
		{
			ExprUnary* expr = (ExprUnary*)expression;
			printTokenType(expr->operator);
			printAst(expr->operand);
			break;
		}

		case EXPR_GROUPING:
		{
			ExprGrouping* expr = (ExprGrouping*)expression;
			printf("(");
			printAst(expr->expression);
			printf(")");
			break;
		}

		default:
			ASSERT_NOT_REACHED();
	}
}