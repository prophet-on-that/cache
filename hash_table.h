#include <stdint.h>

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

int hash_table_put(HashTable *ht, Key *key, Val *val);

Val *hash_table_get(HashTable *ht, Key *key);

int hash_table_delete(HashTable *ht, Key *key);


