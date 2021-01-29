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

bool cmp_vals(struct Val *val, struct Val *other) {
  return val->val_size == other->val_size && !memcmp(val->val, other->val, val->val_size);
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

/*********/
/* TESTS */
/*********/

#define MAX_TESTS 100
#define TEST_HT_SIZE 5

/* The following two keys conflict when hashed, modulo TEST_HT_SIZE */
#define TEST_KEY 0
#define TEST_OTHER_KEY 5

void (*tests[MAX_TESTS])();
int test_count = 0;

void register_test(void (*test)()) {
  assert(test_count < MAX_TESTS);
  tests[test_count++] = test;
}

void run_tests() {
  for (int i = 0; i < test_count; i++)
    tests[i]();
}

/* Construct a single-byte key */
Key *get_key(uint8_t n) {
  uint8_t *buf = malloc(sizeof(uint8_t));
  buf[0] = n;
  Key *key = malloc(sizeof(Key));
  key->key_size = 1;
  key->key = buf;
  return key;
}

/* Construct a single-byte val */
Val *get_val(uint8_t n) {
  uint8_t *buf = malloc(sizeof(uint8_t));
  buf[0] = n;
  Val *val = malloc(sizeof(struct Val));
  val->val_size = 1;
  val->val = buf;
  return val;
}

void test_init() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  assert(ht->size == TEST_HT_SIZE);
  assert(ht->item_count == 0);
  for (unsigned int i = 0; i < TEST_HT_SIZE; i++)
    assert(ht->arr[i] == 0);
}

void test_get_unknown() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  assert(hash_table_get(ht, get_key(TEST_KEY)) == 0);
}

void test_put() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_put_overwrite() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  val = get_val(2);
  assert(hash_table_put(ht, key, val) == 1);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_put_conflict(void) {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  Val *other_val = get_val(2);
  assert(hash_table_put(ht, key, val) == 0);
  assert(hash_table_put(ht, other_key, other_val) == 0);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(cmp_vals(hash_table_get(ht, other_key), other_val));
  assert(ht->item_count == 2);
}

void test_delete_not_present(void) {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  assert(hash_table_delete(ht, other_key));
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_delete(void) {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  Val *other_val = get_val(2);
  assert(hash_table_put(ht, key, val) == 0);
  assert(hash_table_put(ht, other_key, other_val) == 0);
  assert(ht->item_count == 2);

  /* Delete key */
  assert(!hash_table_delete(ht, key));
  assert(!hash_table_get(ht, key));
  assert(cmp_vals(hash_table_get(ht, other_key), other_val));
  assert(ht->item_count == 1);

  /* Delete other_key */
  assert(!hash_table_delete(ht, other_key));
  assert(!hash_table_get(ht, other_key));
  assert(ht->item_count == 0);
}

int main(void) {
  register_test(&test_init);
  register_test(&test_get_unknown);
  register_test(&test_put);
  register_test(&test_put_overwrite);
  register_test(&test_put_conflict);
  register_test(&test_delete_not_present);
  register_test(&test_delete);
  run_tests();

  return 0;
}
