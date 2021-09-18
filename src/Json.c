#include "Json.h"
#include "Assert.h"

//static void copyJsonValue(JsonValue* dst, JsonValue* src)
//{
//	switch (switch_on)
//	{
//	default:
//		break;
//	}
//}
//
//ARRAY_TEMPLATE_DEFINITION(JsonArray, Json, )

void JsonValueFree(JsonValue* value)
{
	switch (value->type)
	{
		case JSON_STRING:
		case JSON_BOOL:
		case JSON_NULL:
		case JSON_NUMBER:
			break;

		case JSON_ARRAY:
			for (size_t i = 0; i < value->value.)
			break;
		case JSON_OBJECT:
			break;
	default:
		ASSERT_NOT_REACHED();
		break;
	}
}
