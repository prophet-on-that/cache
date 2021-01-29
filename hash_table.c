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

typedef uint8_t key_size_t;
typedef uint16_t val_size_t;

struct key_t {
  key_size_t key_size;
  uint8_t *key;
};

/* Create a new key_t by copying the given buffer */
struct key_t *make_key_t(key_size_t size, uint8_t *buf) {
  uint8_t *key_buf = malloc(sizeof(uint8_t) * size);
  memcpy(key_buf, buf, size);
  struct key_t *key = malloc(sizeof(struct key_t *));
  key->key_size = size;
  key->key = key_buf;
  return key;
}

void free_key_t(struct key_t *key) {
  free(key->key);
  free(key);
}

typedef struct val_t {
  val_size_t val_size;
  uint8_t *val;
} val_t;

/* Create a new val_t by copying the given buffer */
val_t *make_val_t(val_size_t size, uint8_t *buf) {
  uint8_t *val_buf = malloc(sizeof(uint8_t) * size);
  memcpy(val_buf, buf, size);
  val_t *val = malloc(sizeof(val_t *));
  val->val_size = size;
  val->val = val_buf;
  return val;
}

void free_val_t(val_t *val) {
  free(val->val);
  free(val);
}

typedef struct list_t {
  struct list_t *next;
  struct key_t *key;
  val_t *val;
} list_t;

void free_list_t(list_t *elem) {
  free_key_t(elem->key);
  free_val_t(elem->val);
  free(elem);
}

/*
 * djb2 hash (see http://www.cse.yorku.ca/~oz/hash.html)
 */
unsigned long hash(struct key_t *key) {
  unsigned long hash = 5381;
  for (unsigned int i = 0; i < key->key_size; i++)
    hash = ((hash << 5) + hash) + key->key[i]; /* hash * 33 + key->key[i] */
  return hash;
}

typedef struct hash_table_t {
  unsigned int size;
  unsigned  item_count;
  list_t **arr;
} hash_table_t;

/* Construct a new hash table of fixed size SIZE. */
hash_table_t *hash_table_new(unsigned int size) {
  list_t **arr = malloc(sizeof(list_t *) * size);
  assert(arr != 0);
  memset(arr, 0, size);
  hash_table_t *ht = malloc(sizeof(hash_table_t));
  assert(ht != 0);
  ht->size = size;
  ht->item_count = 0;
  ht->arr = arr;
  return ht;
}

bool cmp_keys(struct key_t *key, struct key_t *other) {
  return key->key_size == other->key_size && !memcmp(key->key, other->key, key->key_size);
}

bool cmp_vals(struct val_t *val, struct val_t *other) {
  return val->val_size == other->val_size && !memcmp(val->val, other->val, val->val_size);
}

/*
 * Store a value for the given key in a hash table. The key and value
 * pointers are COPIED.
 *
 * Returns 0 if new entry added, 1 if existing entry updated.
 */
int hash_table_put(hash_table_t *ht, struct key_t *key, val_t *val) {
  list_t **ptr = &ht->arr[hash(key) % ht->size];
  list_t *elem;
  while (elem = *ptr) {
    if (cmp_keys(key, elem->key)) {
      /* Update existing elem */
      /* Free existing key if different instance from current */
      free_val_t(elem->val);
      elem->val = make_val_t(val->val_size, val->val);
      return 1;
    }
    ptr = &(*ptr)->next;
  }
  /* Append new list item */
  elem = malloc(sizeof(list_t));
  assert(elem != 0);
  elem->next = 0;
  elem->key = make_key_t(key->key_size, key->key);
  elem->val = make_val_t(val->val_size, val->val);
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
val_t *hash_table_get(hash_table_t *ht, struct key_t *key) {
  list_t **ptr = &ht->arr[hash(key) % ht->size];
  list_t *elem;
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
int hash_table_delete(hash_table_t *ht, struct key_t *key) {
 list_t **ptr = &ht->arr[hash(key) % ht->size];
 list_t *elem;
 while (elem = *ptr) {
   if (cmp_keys(key, elem->key)) {
     *ptr = elem->next;
     free_list_t(elem);
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
struct key_t *get_key(uint8_t n) {
  uint8_t *buf = malloc(sizeof(uint8_t));
  buf[0] = n;
  struct key_t *key = malloc(sizeof(struct key_t));
  key->key_size = 1;
  key->key = buf;
  return key;
}

/* Construct a single-byte val */
val_t *get_val(uint8_t n) {
  uint8_t *buf = malloc(sizeof(uint8_t));
  buf[0] = n;
  val_t *val = malloc(sizeof(struct val_t));
  val->val_size = 1;
  val->val = buf;
  return val;
}

void test_init() {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  assert(ht->size == TEST_HT_SIZE);
  assert(ht->item_count == 0);
  for (unsigned int i = 0; i < TEST_HT_SIZE; i++)
    assert(ht->arr[i] == 0);
}

void test_get_unknown() {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  assert(hash_table_get(ht, get_key(TEST_KEY)) == 0);
}

void test_put() {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  struct key_t *key = get_key(TEST_KEY);
  val_t *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_put_overwrite() {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  struct key_t *key = get_key(TEST_KEY);
  val_t *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  val = get_val(2);
  assert(hash_table_put(ht, key, val) == 1);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_put_conflict(void) {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  struct key_t *key = get_key(TEST_KEY);
  struct key_t *other_key = get_key(TEST_OTHER_KEY);
  val_t *val = get_val(1);
  val_t *other_val = get_val(2);
  assert(hash_table_put(ht, key, val) == 0);
  assert(hash_table_put(ht, other_key, other_val) == 0);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(cmp_vals(hash_table_get(ht, other_key), other_val));
  assert(ht->item_count == 2);
}

void test_delete_not_present(void) {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  struct key_t *key = get_key(TEST_KEY);
  struct key_t *other_key = get_key(TEST_OTHER_KEY);
  val_t *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  assert(hash_table_delete(ht, other_key));
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_delete(void) {
  hash_table_t *ht = hash_table_new(TEST_HT_SIZE);
  struct key_t *key = get_key(TEST_KEY);
  struct key_t *other_key = get_key(TEST_OTHER_KEY);
  val_t *val = get_val(1);
  val_t *other_val = get_val(2);
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
