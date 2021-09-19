#pragma once

#include "String.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// #ifdef moveItemType move #else copyItemType
// hashKeyType, compareKeyType, copyKeyType, copyItemType, freeKeyType freeItemType
// mark things const also in array

#define TABLE_INITIAL_CAPACITY 16
#define TABLE_MAX_LOAD 0.75

#define TABLE_TEMPLATE_DECLARATION(tableTypeName, keyType, valueType) \
typedef struct tableTypeName##Entry \
{ \
	keyType key; \
	valueType value; \
	struct tableTypeName##Entry* next; \
} tableTypeName##Entry; \
\
typedef struct \
{ \
	tableTypeName##Entry* data; \
	size_t size; \
	size_t capacity; \
} tableTypeName##; \
\
void tableTypeName##Init(tableTypeName* table); \
void tableTypeName##Free(tableTypeName* table); \
void tableTypeName##Set(tableTypeName* table, keyType* key, valueType value); \
bool tableTypeName##Get(tableTypeName* table, keyType* key, valueType* result); \
bool tableTypeName##Remove(tableTypeName* table, const keyType* key);

// size_t hashKeyType(const keyType* key);
// void copyKeyType(keyType* destination, const keyType* source);
// void freeKeyType(keyType* key);
#define TABLE_TEMPLATE_DEFINITION(tableTypeName, keyType, valueType, hashKeyType, copyKeyType, compareKeyType, freeKeyType, copyValueType, freeValueType, isKeyNull, setKeyNull) \
void tableTypeName##Init(tableTypeName* table) \
{ \
    table->capacity = TABLE_INITIAL_CAPACITY; \
    table->data = malloc(table->capacity * sizeof(tableTypeName##Entry)); \
    if (table->data == NULL) \
    { \
        fputs("Failed to allocate table", stderr); \
        exit(1); \
    } \
    for (size_t i = 0; i < table->capacity; i++) \
    { \
        table->data[i].next = NULL; \
        setKeyNull(&table->data[i].key); \
    } \
    table->size = 0; \
} \
\
static size_t grow##tableTypeName##Capacity(size_t capacity) \
{ \
    return capacity * 2; \
} \
\
static void copy##tableTypeName##Entry(tableTypeName##Entry* dst, tableTypeName##Entry* src) \
{ \
    copyKeyType(&dst->key, &src->key); \
    copyValueType(&dst->value, &src->value); \
} \
\
static void free##tableTypeName##Entry(tableTypeName##Entry* entry) \
{ \
    if (entry->next == NULL) \
    { \
        freeKeyType(&entry->key);\
        freeValueType(&entry->value); \
    } \
    else \
    { \
        freeKeyType(&entry->key); \
        freeValueType(&entry->value); \
        tableTypeName##Entry* e = entry->next; \
        while (e->next != NULL) \
        { \
            tableTypeName##Entry* next = e->next; \
            freeKeyType(&e->key); \
            freeValueType(&e->value); \
            free(e); \
            e = next; \
        } \
    } \
} \
\
static void grow##tableTypeName(tableTypeName* table) \
{ \
    size_t newCapacity = grow##tableTypeName##Capacity(table->capacity); \
    tableTypeName##Entry* newData = malloc(newCapacity * sizeof(tableTypeName##Entry)); \
    if (newData == NULL) \
    { \
        fputs("Failed to reallocate table", stderr); \
        exit(1);\
    } \
    for (size_t i = 0; i < newCapacity; i++) \
    { \
        newData[i].next = NULL; \
        setKeyNull(&newData[i].key); \
    } \
\
    for (size_t i = 0; i < table->capacity; i++) \
    { \
        if (isKeyNull(&table->data[i].key) == false) \
        { \
            tableTypeName##Entry* oldEntry = &table->data[i]; \
            size_t hash = hashKeyType(&oldEntry->key) % newCapacity; \
            while (oldEntry != NULL) \
            { \
                tableTypeName##Entry* newEntry = &newData[hash]; \
                if (isKeyNull(&newEntry->key)) \
                { \
                    copy##tableTypeName##Entry(newEntry, oldEntry); \
                    newEntry->next = NULL; \
                } \
                else \
                { \
                    while (newEntry->next != NULL) \
                    { \
                        newEntry = newEntry->next; \
                    } \
                    newEntry->next = malloc(sizeof(tableTypeName##Entry)); \
                    if (newEntry->next == NULL) \
                    { \
                        fputs("Failed to allocate table entry", stderr); \
                        exit(1); \
                    } \
                    copy##tableTypeName##Entry(newEntry->next, oldEntry); \
                    newEntry->next->next = NULL; \
                } \
\
                oldEntry = oldEntry->next; \
            } \
        } \
    } \
    for (size_t i = 0; i < table->capacity; i++) \
    { \
        if (isKeyNull(&table->data[i].key) == false) \
        { \
            free##tableTypeName##Entry(&table->data[i]); \
        } \
    } \
    free(table->data); \
    table->data = newData; \
    table->capacity = newCapacity; \
} \
\
void tableTypeName##Set(tableTypeName* table, keyType* key, valueType value) \
{ \
    if (((double)table->size / (double)table->capacity) > 0.75) \
    { \
        grow##tableTypeName(table); \
    } \
    size_t index = hashKeyType(key) % table->capacity; \
    if (isKeyNull(&table->data[index].key)) \
    { \
        copyKeyType(&table->data[index].key, key); \
        copyValueType(&table->data[index].value, &value); \
        table->data[index].next = NULL; \
    } \
    else \
    { \
        tableTypeName##Entry * entry = &table->data[index]; \
        while (entry->next != NULL) \
        { \
            entry = entry->next; \
        } \
        entry->next = malloc(sizeof(tableTypeName##Entry)); \
        if (entry->next == NULL) \
        { \
            fputs("Failed to allocate table entry", stderr); \
            exit(1); \
        } \
        copyKeyType(&entry->next->key, key); \
        copyValueType(&entry->next->value, &value); \
        entry->next->next = NULL; \
    } \
    table->size++; \
} \
\
bool tableTypeName##Get(tableTypeName* table, keyType* key, valueType* result) \
{ \
    size_t index = hashKeyType(key) % table->capacity; \
    tableTypeName##Entry* entry = &table->data[index]; \
    if (isKeyNull(&entry->key)) \
    { \
        return false; \
    } \
    while (entry != NULL) \
    { \
        if (compareKeyType(&entry->key, key)) \
        { \
            copyValueType(result, &entry->value); \
            return true; \
        } \
        entry = entry->next; \
    } \
    return false; \
} \
\
bool tableTypeName##Remove(tableTypeName* table, const keyType* key) { \
    size_t index = hashKeyType(key) % table->capacity; \
    tableTypeName##Entry* entry = &table->data[index]; \
    if (isKeyNull(&entry->key)) \
    { \
        return false; \
    } \
    if (compareKeyType(&entry->key, key)) \
    { \
        setKeyNull(&entry->key); \
        table->size--; \
        return true; \
    } \
    tableTypeName##Entry* previous = entry; \
    entry = entry->next; \
    while (entry != NULL) \
    { \
        if (compareKeyType(&entry->key, key)) \
        { \
            previous->next = entry->next; \
            freeKeyType(&entry->key); \
            freeValueType(&entry->value); \
            free(entry); \
            table->size--; \
            return true; \
        } \
        previous = entry; \
        entry = entry->next; \
    } \
    return false; \
} \
\
void tableTypeName##Free(tableTypeName* table) \
{ \
    for (size_t i = 0; i < table->capacity; i++) \
    { \
        if (isKeyNull(&table->data[i].key) == false) \
        { \
            free##tableTypeName##Entry(&table->data[i]); \
        } \
    } \
    free(table->data); \
}