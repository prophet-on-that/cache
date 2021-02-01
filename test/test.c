#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../lib/hash_table.h"

/**************/
/* Test utils */
/**************/

#define MAX_TESTS 500
#define TEST_HT_SIZE 5

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

/********************/
/* hash_table tests */
/********************/

/* The following two keys conflict when hashed, modulo TEST_HT_SIZE */
#define TEST_KEY 0
#define TEST_OTHER_KEY 5

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

bool cmp_vals(struct Val *val, struct Val *other) {
  return val->val_size == other->val_size && !memcmp(val->val, other->val, val->val_size);
}

void test_ht_init() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  assert(ht->size == TEST_HT_SIZE);
  assert(ht->item_count == 0);
  for (unsigned int i = 0; i < TEST_HT_SIZE; i++)
    assert(ht->arr[i] == 0);
}

void test_ht_get_unknown() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  assert(hash_table_get(ht, get_key(TEST_KEY)) == 0);
}

void test_ht_put() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_ht_put_overwrite() {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  val = get_val(2);
  assert(hash_table_put(ht, key, val) == 1);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_ht_put_conflict(void) {
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

void test_ht_delete_not_present(void) {
  HashTable *ht = hash_table_new(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == 0);
  assert(hash_table_delete(ht, other_key));
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_ht_delete(void) {
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

/********/
/* Main */
/********/

int main(void) {
  register_test(&test_ht_init);
  register_test(&test_ht_get_unknown);
  register_test(&test_ht_put);
  register_test(&test_ht_put_overwrite);
  register_test(&test_ht_put_conflict);
  register_test(&test_ht_delete_not_present);
  register_test(&test_ht_delete);
  run_tests();
  return 0;
}
