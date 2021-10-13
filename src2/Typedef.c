//#include "Typedef.h"
//#include "Compiler.h"
//
//void declareTypedef(Compiler* compiler, StringView name, const DataType* dataType)
//{
//	for (size_t i = 0; i < compiler->typedefTable.size; i++)
//	{
//		if (StringViewEquals(&compiler->typedefTable.data[i].name, &name))
//		{
//			compiler->typedefTable.data[i].name = name;
//			compiler->typedefTable.data[i].type = *dataType;
//			return;
//		}
//	}
//
//	Typedef typdef;
//	typdef.name = name;
//	typdef.type = *dataType;
//	TypedefArrayAppend(&compiler->typedefTable, typdef);
//}
//
//bool resolveTypedef(Compiler* compiler, StringView name, DataType* dataType)
//{
//	for (size_t i = 0; i < compiler->typedefTable.size; i++)
//	{
//		if (StringViewEquals(&compiler->typedefTable.data[i].name, &name))
//		{
//			*dataType = compiler->typedefTable.data[i].type;
//			return true;
//		}
//	}
//
//	dataType->type = DATA_TYPE_ERROR;
//	return false;
//}
