#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_INITIAL_CAPACITY 4

//#define GENERIC_FREE_NO_OP, 

#define ARRAY_TEMPLATE_DECLARATION(arrayTypeName, itemType) \
typedef struct \
{ \
	size_t size; \
	size_t capacity; \
	itemType* data; \
} arrayTypeName; \
void arrayTypeName##Init(arrayTypeName* array); \
void arrayTypeName##Free(arrayTypeName* array); \
void arrayTypeName##Append(arrayTypeName* array, itemType item);

// void copyItemType(itemType* destination, itemType* source)
// void freeItemType(itemType* ptr)
#define ARRAY_TEMPLATE_DEFINITION(arrayTypeName, itemType, copyItemType, freeItemType) \
\
static int growCapacity(size_t capacity) \
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
		array->capacity = growCapacity(array->capacity); \
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
}