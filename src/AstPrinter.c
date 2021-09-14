#include "AstPrinter.h"
#include "Assert.h"

#include <stdarg.h>

#define PRETTY_PRINT

static void printTabs(int count)
{
#ifdef PRETTY_PRINT
	for (int i = 0; i < count; i++)
		putchar('\t');
#endif
}

static void printObjectStart(int depth)
{
	putchar('{');
#ifdef PRETTY_PRINT
	putchar('\n');
#endif 
}

static void printObjectEnd(int depth)
{
	printTabs(depth);
	putchar('}');
#ifdef PRETTY_PRINT
	putchar('\n');
#endif
}

static void printArrayStart(int depth)
{
	putchar('{');
#ifdef PRETTY_PRINT
	putchar('\n');
#endif 
}

static void printSeparator()
{
	putchar(':');
#ifdef PRETTY_PRINT
	putchar(' ');
#endif
}

static void printMemberSeparator()
{
	putchar(',');
#ifdef PRETTY_PRINT
	putchar('\n');
#endif
}

static void printKey(const char* key, int depth)
{
	printTabs(depth);
	printf(key);
	putchar(':');
#ifdef PRETTY_PRINT
	putchar(' ');
#endif
}

static void printMember(int depth, const char* key, const char* format, ...)
{
	printTabs(depth);
	printKey(key, depth);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
#ifdef PRETTY_PRINT
	putchar('\n');
#endif
}

static void printArrayEnd(int depth)
{
	printTabs(depth);
	putchar('}');
#ifdef PRETTY_PRINT
	putchar('\n');
#endif
}

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

const char* TokenTypeToString(TokenType type)
{
	switch (type)
	{
		case TOKEN_NUMBER: return "TOKEN_NUMBER";
		case TOKEN_PLUS: return "TOKEN_PLUS";
		case TOKEN_MINUS: return "TOKEN_MINUS";
		case TOKEN_ASTERISK: return "TOKEN_ASTERISK";
		case TOKEN_SLASH: return "TOKEN_SLASH";
		case TOKEN_PERCENT: return "TOKEN_PERCENT";
		case TOKEN_LEFT_PAREN: return "TOKEN_LEFT_PAREN";
		case TOKEN_RIGHT_PAREN: return "TOKEN_RIGHT_PAREN";
		case TOKEN_EOF: return "TOKEN_EOF";
		case TOKEN_ERROR: return "TOKEN_ERROR";
		case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
		case TOKEN_IF: return "TOKEN_IF";
		case TOKEN_INT: return "TOKEN_INT";
		case TOKEN_DOUBLE: return "TOKEN_DOUBLE";
		case TOKEN_DO: return "TOKEN_DO";

		default:
			ASSERT_NOT_REACHED();
	}
	return "";
}

const char* ExprTypeToString(ExprType type)
{
	switch (type)
	{

	default:
		ASSERT_NOT_REACHED();
	}

	return "";
}

void printExpr(Expr* expression, int depth)
{
	switch (expression->type)
	{
		case EXPR_BINARY:
		{
			ExprBinary* expr = (ExprBinary*)expression;
			printObjectStart(depth);
			printKey("left", depth + 1);
			printExpr(expr->left, depth + 1);
			printMember(depth, "operator", TokenTypeToString(expr->operator));
			printKey("right", depth + 1);
			printExpr(expr->right, depth + 1);
			printObjectEnd(depth);
			break;
		}

		case EXPR_INT_LITERAL:
		{
			ExprIntLiteral* expr = (ExprIntLiteral*)expression;
			printf("%.*s", expr->literal.length, expr->literal.chars);
			printMemberSeparator();
			break;
		}

		case EXPR_UNARY:
		{
			ExprUnary* expr = (ExprUnary*)expression;
			printObjectStart(depth);
			printTokenType(expr->operator);
			printExpr(expr->operand, depth + 1);
			printObjectEnd(depth);
			break;
		}

		case EXPR_GROUPING:
		{
			ExprGrouping* expr = (ExprGrouping*)expression;
			printf("(");
			printExpr(expr->expression, depth + 1);
			printf(")");
			break;
		}

		default:
			ASSERT_NOT_REACHED();
			break;
	}
}

void printStmt(Stmt* statement, int depth)
{
	switch (statement->type)
	{
	case STMT_EXPRESSION:
		printObjectStart(depth);
		printMember(depth, "type", "STMT_EXPRESSION");
		printObjectEnd(depth);
		break;

	case STMT_VARIABLE_DECLARATION:
		break;

	default:
		ASSERT_NOT_REACHED();
		break;
	}
}

#undef PRETTY_PRINT