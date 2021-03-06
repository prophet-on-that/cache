#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../lib/hash_table.h"
#include "../lib/message.h"
#include "../lib/conn.h"

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
#define TEST_KEY 5
#define TEST_OTHER_KEY 0
#define TEST_VAL 2
#define TEST_OTHER_VAL 17

/* Initialise a single-byte key */
void init_key(Key *key, uint8_t n) {
  uint8_t *buf = malloc(sizeof(uint8_t));
  buf[0] = n;
  key->key_size = 1;
  key->key = buf;
}

/* Construct a single-byte key */
Key *get_key(uint8_t n) {
  Key *key = malloc(sizeof(Key));
  init_key(key, n);
  return key;
}

/* Initialise a single-byte val */
void init_val(Val *val, uint8_t n) {
  uint8_t *buf = malloc(sizeof(uint8_t));
  buf[0] = n;
  val->val_size = 1;
  val->val = buf;
}

/* Construct a single-byte val */
Val *get_val(uint8_t n) {
  Val *val = malloc(sizeof(struct Val));
  init_val(val, n);
  return val;
}

bool cmp_vals(struct Val *val, struct Val *other) {
  return val->val_size == other->val_size && !memcmp(val->val, other->val, val->val_size);
}

void test_ht_init() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  assert(ht->size == TEST_HT_SIZE);
  assert(ht->item_count == 0);
  for (unsigned int i = 0; i < TEST_HT_SIZE; i++)
    assert(ht->arr[i] == 0);
}

void test_ht_get_unknown() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  assert(hash_table_get(ht, get_key(TEST_KEY)) == NULL);
}

void test_ht_put() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == false);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_ht_put_overwrite() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == false);
  val = get_val(2);
  assert(hash_table_put(ht, key, val) == true);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_ht_put_conflict(void) {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  Val *other_val = get_val(2);
  assert(hash_table_put(ht, key, val) == false);
  assert(hash_table_put(ht, other_key, other_val) == false);
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(cmp_vals(hash_table_get(ht, other_key), other_val));
  assert(ht->item_count == 2);
}

void test_ht_delete_not_present(void) {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  assert(hash_table_put(ht, key, val) == false);
  assert(hash_table_delete(ht, other_key));
  assert(cmp_vals(hash_table_get(ht, key), val));
  assert(ht->item_count == 1);
}

void test_ht_delete(void) {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Key *other_key = get_key(TEST_OTHER_KEY);
  Val *val = get_val(1);
  Val *other_val = get_val(2);
  assert(hash_table_put(ht, key, val) == false);
  assert(hash_table_put(ht, other_key, other_val) == false);
  assert(ht->item_count == 2);

  /* Delete key */
  assert(!hash_table_delete(ht, key));
  assert(hash_table_get(ht, key) == NULL);
  assert(cmp_vals(hash_table_get(ht, other_key), other_val));
  assert(ht->item_count == 1);

  /* Delete other_key */
  assert(!hash_table_delete(ht, other_key));
  assert(!hash_table_get(ht, other_key));
  assert(ht->item_count == 0);
}

/*****************/
/* message tests */
/*****************/

void test_msg_serialise_get() {
  Message msg;
  init_key(&msg.message.get.key, 0);
  msg.type = GET;
  size_t buf_size;
  uint8_t *buf = out_serialise_message(&msg, &buf_size);
  Message *msg_copy = out_deserialise_message(buf + sizeof(MessageSize), buf_size - sizeof(MessageSize));
  assert(msg_copy->type == GET);
  assert(cmp_keys(&msg_copy->message.get.key, &msg.message.get.key));
}

void test_msg_serialise_put() {
  Message msg;
  init_key(&msg.message.put.key, 1);
  init_val(&msg.message.put.val, 2);
  msg.type = PUT;
  size_t buf_size;
  uint8_t *buf = out_serialise_message(&msg, &buf_size);
  Message *msg_copy = out_deserialise_message(buf + sizeof(MessageSize), buf_size - sizeof(MessageSize));
  assert(msg_copy->type == PUT);
  assert(cmp_keys(&msg_copy->message.put.key, &msg.message.put.key));
  assert(msg_copy->message.put.val.val_size == 1);
  assert(msg_copy->message.put.val.val[0] == 2);
}

void test_msg_serialise_get_resp() {
  Message msg;
  msg.message.get_resp.val = malloc(sizeof(Val));
  init_val(msg.message.get_resp.val, 2);
  msg.type = GET_RESP;
  size_t buf_size;
  uint8_t *buf = out_serialise_message(&msg, &buf_size);
  Message *msg_copy = out_deserialise_message(buf + sizeof(MessageSize), buf_size - sizeof(MessageSize));
  assert(msg_copy->type == GET_RESP);
  assert(cmp_vals(msg_copy->message.get_resp.val, msg.message.get_resp.val));
  assert(msg_copy->message.get_resp.val->val_size == 1);
  assert(msg_copy->message.get_resp.val->val[0] == 2);
}

void test_msg_serialise_get_resp_null() {
  Message msg;
  msg.message.get_resp.val = NULL;
  msg.type = GET_RESP;
  size_t buf_size;
  uint8_t *buf = out_serialise_message(&msg, &buf_size);
  Message *msg_copy = out_deserialise_message(buf + sizeof(MessageSize), buf_size - sizeof(MessageSize));
  assert(msg_copy->type == GET_RESP);
  assert(msg_copy->message.get_resp.val == NULL);
}

/**************/
/* conn tests */
/**************/

void test_conn_handle_get() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Key *key = get_key(TEST_KEY);
  Val *val = get_val(1);
  hash_table_put(ht, key, val);
  Message msg;
  init_key(&msg.message.get.key, TEST_KEY);
  msg.type = GET;
  Message *resp = out_handle_msg(&msg, ht);
  assert(resp->type == GET_RESP);
  assert(cmp_vals(val, resp->message.get_resp.val));
}

void test_conn_handle_get_unknown() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Message msg;
  init_key(&msg.message.get.key, TEST_KEY);
  msg.type = GET;
  Message *resp = out_handle_msg(&msg, ht);
  assert(resp->type == GET_RESP);
  assert(resp->message.get_resp.val == NULL);
}

void test_conn_handle_put() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Message msg;
  init_key(&msg.message.put.key, TEST_KEY);
  init_val(&msg.message.put.val, TEST_VAL);
  msg.type = PUT;
  Message *resp = out_handle_msg(&msg, ht);
  assert(resp->type == PUT_RESP);
  assert(resp->message.put_resp.is_update == false);
  assert(cmp_vals(hash_table_get(ht, get_key(TEST_KEY)), get_val(TEST_VAL)));
}

void test_conn_handle_put_update() {
  HashTable *ht = create_hash_table(TEST_HT_SIZE);
  Message msg;
  init_key(&msg.message.put.key, TEST_KEY);
  hash_table_put(ht, &msg.message.put.key, get_val(TEST_VAL));
  init_val(&msg.message.put.val, TEST_OTHER_VAL);
  msg.type = PUT;
  Message *resp = out_handle_msg(&msg, ht);
  assert(resp->type == PUT_RESP);
  assert(resp->message.put_resp.is_update == true);
  assert(cmp_vals(hash_table_get(ht, get_key(TEST_KEY)), get_val(TEST_OTHER_VAL)));
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
  register_test(&test_msg_serialise_get);
  register_test(&test_msg_serialise_put);
  register_test(&test_msg_serialise_get_resp);
  register_test(&test_msg_serialise_get_resp_null);
  register_test(&test_conn_handle_get);
  register_test(&test_conn_handle_get_unknown);
  register_test(&test_conn_handle_put);
  register_test(&test_conn_handle_put_update);
  run_tests();
  return 0;
}
