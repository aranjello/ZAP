#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity,
                        Key* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
        Array *a = entry->value;
        if (entry->value == NULL)
        {
            // Empty entry.
            return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

bool tableGet(Table* table, struct Key* key, struct Array* value) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  value = entry->value;
  return true;
}

static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NULL;
  }
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(Table* table, Key* key, struct Array* value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }
  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && entry->value == NULL) table->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

bool tableDelete(Table* table, Key* key) {
  if (table->count == 0) return false;

  // Find the entry.
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = NULL;
  return true;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

po tableFindKey(Table* table, const char* chars,
                           int length, uint32_t hash) {
  po p;
  if (table->count == 0) return NULL;
  for (int i = 0; i < table->capacity; i++){
    if(&table->entries[i].key != NULL){
      printf("key at %d in find is %s\n", i, &table->entries[i].key->value);
    }
  }
    uint32_t index = hash % table->capacity;
  for (;;) {
    Entry* entry = &table->entries[index];
    
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (entry->value == NULL) return po p{.ptr = NULL,.offset = 0};
      
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->value, chars, length) == 0) {
      // We found it.
      po p;
      p.ptr = entry->key;
      p.offset = index;
      return p;
    }

    index = (index + 1) % table->capacity;
  }
}