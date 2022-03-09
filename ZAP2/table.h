#ifndef ZAP_table_h
#define ZAP_table_h

#include "value.h"
#include "codeChunk.h"
#include "common.h"

//A key value pair that will be stored in a table
typedef struct Entry{
  Key* key;
  Array* value;
} Entry;

//A table to store entries
typedef struct Table{
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);

bool tableSet(Table* table, Key* key,  Array* value);
Array* tableGet(Table* table, Key* key);
bool tableDelete(Table* table,  Key* key);

po tableFindKey(Table* table, const char* chars,
                           int length, uint32_t hash);

void tableAddAll(Table* from, Table* to);
#endif