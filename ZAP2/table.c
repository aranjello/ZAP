#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "memory.h"
#include "table.h"

//Sets the threshold for when the table needs to be reesized to reduce collisions
#define TABLE_MAX_LOAD 0.75

//Allocates a new pointer for the given type of information with the given capacity
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

/*
Initializes an empty table
@param table The table to initialize
*/
void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

/*
frees the data contined in a table
@param table The table to free
*/
void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

/*
Finds an entry in the given table given a list of entries, a capacity and a key
@param entries The list of entries to search through
@param capacity The capacity of the table the entries come from
@param key The key to serach for
@return Returns a po struct that contains a pointer to the data if found as well as its
offset in its containign array and NULL and 0 if not found
*/
static po findEntry(Entry* entries, int capacity,
                        Key* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;
  po p;
  p.ptr = NULL;
  p.offset = 0;
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (entry->value == NULL)
      {
            // Empty entry.
            p.ptr = (tombstone != NULL) ? tombstone : entry;
            return p;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      p.ptr = entry;
      p.offset = entry->key->loc;
      return p;
    }

    index = (index + 1) % capacity;
  }
}

/*
Gets a pointer to the data associated with some key in some table
@param table The table to search
@param key The key to search for
*/
Array * tableGet(Table* table, Key* key) {
  if (table->count == 0) return false;
  
  Entry* entry = findEntry(table->entries, table->capacity, key).ptr;
  if (entry->key == NULL) return false;

  return entry->value;
}

/*
adjusts the capacity of a given table
@param table The table to adjust
@param capacity The capacity of the table after adjustment
*/
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

    Entry* dest = findEntry(entries, capacity, entry->key).ptr;
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

/*
Stores a new key value pair in the given table
@param table The table to store the data in
@param key The key for the new pair
@param value The value for the new pair
*/
bool tableSet(Table* table, Key* key, Array* value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }
  Entry* entry = findEntry(table->entries, table->capacity, key).ptr;
  bool isNewKey = entry->key == NULL;
  if (isNewKey && entry->value == NULL) table->count++;

  entry->key = key;
  
  entry->value = value;
  return isNewKey;
  
}

/*
"Removes" a value from the table by setting it to be a tombstone
@param table The table to remove from
@param key The key to remove
*/
bool tableDelete(Table* table, Key* key) {
  if (table->count == 0) return false;

  // Find the entry.
  Entry* entry = findEntry(table->entries, table->capacity, key).ptr;
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = NULL;
  return true;
}

/*
Moves all entries from one table to another
@param from The table to move entries from
@param to The table to move entries to
*/
void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

/*
Checks the table to see if a key has already been entered and returns it if
it has. This is used for string interning so that the same string in two differnt
places will always have the same pointer
@param table The table to search for the key
@param chars The chars of a new string
@param length The length of the new string
@param hash The hash of the new string
*/
po tableFindKey(Table* table, const char* chars,
                           int length, uint32_t hash) {
  po p;
  p.ptr = NULL;
  p.offset = 0;
  if (table->count == 0) return p;
  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry* entry = &table->entries[index];
    
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (entry->value == NULL) return p;
      
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->value, chars, length) == 0) {
      // We found it.
      p.ptr = entry->key;
      p.offset = entry->key->loc;
      return p;
    }

    index = (index + 1) % table->capacity;
  }
}