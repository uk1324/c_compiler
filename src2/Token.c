#include "Token.h"
#include "Generic.h"

static void copyToken(Token* dst, const Token* src)
{
	*dst = *src;
}

ARRAY_TEMPLATE_DEFINITION(TokenArray, Token, copyToken, NO_OP_FUNCTION)