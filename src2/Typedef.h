//#pragma once
//
//#include "StringView.h"
//#include "DataType.h"
//#include "Array.h"
//
//typedef struct Compiler;
//
//typedef struct
//{
//	StringView name;
//	DataType type;
//} Typedef;
//
//ARRAY_TEMPLATE_DECLARATION(TypedefArray, Typedef)
//
//
//void declareTypedef(Compiler* compiler, StringView name, const DataType* dataType);
//// Returns if the typedef exists
//bool resolveTypedef(Compiler* compiler, StringView name, DataType* dataType);