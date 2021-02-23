#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t KeySize;
typedef uint16_t ValSize;

typedef struct Key {
  KeySize key_size;
  uint8_t *key;
} Key;

typedef struct Val {
  ValSize val_size;
  uint8_t *val;
} Val;

typedef struct List {
  struct List *next;
  Key *key;
  Val *val;
} List;

typedef struct HashTable {
  unsigned int size;
  unsigned  item_count;
  List **arr;
} HashTable;

HashTable *hash_table_new(unsigned int size);

bool hash_table_put(HashTable *ht, Key *key, Val *val);

Val *hash_table_get(HashTable *ht, Key *key);

int hash_table_delete(HashTable *ht, Key *key);

size_t key_size(Key *key);

size_t val_size(Val *val);

bool cmp_keys(Key *key, Key *other);

void free_key(Key *key);

void free_val(Val *val);

#endif
