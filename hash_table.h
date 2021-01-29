#include <stdint.h>

typedef uint8_t key_size_t;
typedef uint16_t val_size_t;

struct key_t {
  key_size_t key_size;
  uint8_t *key;
};

typedef struct val_t {
  val_size_t val_size;
  uint8_t *val;
} val_t;

typedef struct list_t {
  struct list_t *next;
  struct key_t *key;
  val_t *val;
} list_t;

typedef struct hash_table_t {
  unsigned int size;
  unsigned  item_count;
  list_t **arr;
} hash_table_t;

hash_table_t *hash_table_new(unsigned int size);

int hash_table_put(hash_table_t *ht, struct key_t *key, val_t *val);

val_t *hash_table_get(hash_table_t *ht, struct key_t *key);

int hash_table_delete(hash_table_t *ht, struct key_t *key);


