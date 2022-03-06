#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct Entry{
  struct Key* key;
  struct Array* value;
} Entry;

typedef struct Table{
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);

bool tableSet(Table* table, struct Key* key, struct Array* value);
bool tableGet(Table* table, struct Key* key, struct Array* value);
bool tableDelete(Table* table, struct Key* key);

Key* tableFindkey(Table* table, const char* chars,
                           int length, uint32_t hash);

void tableAddAll(Table* from, Table* to);
#endif