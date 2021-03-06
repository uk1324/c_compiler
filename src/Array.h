#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Could replace the grow capacity function with just one function for all types

#define ARRAY_INITIAL_CAPACITY 4

#define ARRAY_TEMPLATE_DECLARATION(arrayTypeName, itemType) \
typedef struct \
{ \
	size_t size; \
	size_t capacity; \
	itemType* data; \
} arrayTypeName; \
void arrayTypeName##Init(arrayTypeName* array); \
void arrayTypeName##Free(arrayTypeName* array); \
void arrayTypeName##Append(arrayTypeName* array, itemType item); \
void arrayTypeName##Clear(arrayTypeName* array);

// void copyItemType(itemType* destination, itemType* source)
// void freeItemType(itemType* ptr)
#define ARRAY_TEMPLATE_DEFINITION(arrayTypeName, itemType, copyItemType, freeItemType) \
\
static int grow##arrayTypeName##Capacity(size_t capacity) \
{ \
	return capacity * 2; \
} \
\
void arrayTypeName##Init(arrayTypeName* array) \
{ \
	array->size = 0; \
	array->capacity = ARRAY_INITIAL_CAPACITY;\
	array->data = malloc(array->capacity * sizeof(itemType)); \
} \
\
void arrayTypeName##Free(arrayTypeName* array) \
{ \
	free(array->data); \
} \
\
void arrayTypeName##Append(arrayTypeName* array, itemType item) \
{ \
	if ((array->size + 1) > array->capacity) \
	{ \
		array->capacity = grow##arrayTypeName##Capacity(array->capacity); \
		itemType* newData = malloc(array->capacity * sizeof(itemType)); \
		if (newData == NULL) \
		{ \
			fputs("Failed to reallocate array\n", stderr); \
			exit(1); \
		} \
		for (size_t i = 0; i < array->size; i++) \
		{ \
			copyItemType(&newData[i], &array->data[i]); \
			freeItemType(&array->data[i]); \
		} \
		free(array->data); \
		array->data = newData; \
	} \
	copyItemType(&array->data[array->size], &item); \
	array->size++; \
} \
void arrayTypeName##Clear(arrayTypeName* array) \
{ \
	for (size_t i = 0; i < array->size; i++) \
	{ \
		freeItemType(&array->data[i]); \
	} \
	array->size = 0; \
}