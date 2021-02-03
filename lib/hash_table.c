/*
 * Hash table implementation. The array size of the hash table is
 * fixed. Not thread-safe.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "hash_table.h"

/* Create a new Key by copying the given buffer */
Key *make_key(KeySize size, uint8_t *buf) {
  uint8_t *key_buf = malloc(sizeof(uint8_t) * size);
  memcpy(key_buf, buf, size);
  Key *key = malloc(sizeof(Key *));
  key->key_size = size;
  key->key = key_buf;
  return key;
}

void free_key(Key *key) {
  free(key->key);
  free(key);
}

/* Return serialised size of a key. */
size_t key_size(Key *key) {
  return sizeof(key->key_size) + key->key_size;
}

/* Create a new Val by copying the given buffer */
Val *make_val(ValSize size, uint8_t *buf) {
  uint8_t *val_buf = malloc(sizeof(uint8_t) * size);
  memcpy(val_buf, buf, size);
  Val *val = malloc(sizeof(Val *));
  val->val_size = size;
  val->val = val_buf;
  return val;
}

void free_val(Val *val) {
  free(val->val);
  free(val);
}

/* Return serialised size of a val. */
size_t val_size(Val *val) {
  return sizeof(val->val_size) + val->val_size;
}

void free_list(List *elem) {
  free_key(elem->key);
  free_val(elem->val);
  free(elem);
}

/*
 * djb2 hash (see http://www.cse.yorku.ca/~oz/hash.html)
 */
unsigned long hash(Key *key) {
  unsigned long hash = 5381;
  for (unsigned int i = 0; i < key->key_size; i++)
    hash = ((hash << 5) + hash) + key->key[i]; /* hash * 33 + key->key[i] */
  return hash;
}

/* Construct a new hash table of fixed size SIZE. */
HashTable *hash_table_new(unsigned int size) {
  List **arr = malloc(sizeof(List *) * size);
  assert(arr != 0);
  memset(arr, 0, size);
  HashTable *ht = malloc(sizeof(HashTable));
  assert(ht != 0);
  ht->size = size;
  ht->item_count = 0;
  ht->arr = arr;
  return ht;
}

bool cmp_keys(Key *key, Key *other) {
  return key->key_size == other->key_size && !memcmp(key->key, other->key, key->key_size);
}

/*
 * Store a value for the given key in a hash table. The key and value
 * pointers are COPIED.
 *
 * Returns 0 if new entry added, 1 if existing entry updated.
 */
int hash_table_put(HashTable *ht, Key *key, Val *val) {
  List **ptr = &ht->arr[hash(key) % ht->size];
  List *elem;
  while (elem = *ptr) {
    if (cmp_keys(key, elem->key)) {
      /* Update existing elem */
      /* Free existing key if different instance from current */
      free_val(elem->val);
      elem->val = make_val(val->val_size, val->val);
      return 1;
    }
    ptr = &(*ptr)->next;
  }
  /* Append new list item */
  elem = malloc(sizeof(List));
  assert(elem != 0);
  elem->next = 0;
  elem->key = make_key(key->key_size, key->key);
  elem->val = make_val(val->val_size, val->val);
  *ptr = elem;
  ++ht->item_count;
  return 0;
}

/*
 * Fetch pointer to value for a given key. This is a pointer to the
 * data stored in the hash table, so must be copied if modification is
 * needed.
 *
 * Returns 0 if nothing found.
 */
Val *hash_table_get(HashTable *ht, Key *key) {
  List **ptr = &ht->arr[hash(key) % ht->size];
  List *elem;
  while (elem = *ptr) {
    if (cmp_keys(key, elem->key))
      return elem->val;
    ptr = &(*ptr)->next;
  }
  return 0;
}

/*
 * Delete a key from the hash table.
 *
 * Returns 0 on success, 1 if no elem deleted.
 */
int hash_table_delete(HashTable *ht, Key *key) {
 List **ptr = &ht->arr[hash(key) % ht->size];
 List *elem;
 while (elem = *ptr) {
   if (cmp_keys(key, elem->key)) {
     *ptr = elem->next;
     free_list(elem);
     --ht->item_count;
     return 0;
   }
   ptr = &(*ptr)->next;
 }
 return 1;
}
