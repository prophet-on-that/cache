#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdint.h>
#include "hash_table.h"

typedef uint32_t MessageSize;

typedef struct MessageGet {
  Key *key;
} MessageGet;

typedef struct MessagePut {
  Key key;
  Val val;
} MessagePut;

typedef uint8_t MessageType;

typedef union MessageUnion {
  MessageGet get;
  MessagePut put;
} MessageUnion;

typedef struct Message {
  MessageType type;
  MessageUnion message;
} Message;

#define MessageTypeGet 0
#define MessageTypePut 1

uint8_t *serialise_message(Message *msg, size_t *buf_size);

Message *deserialise_message(uint8_t *buf, size_t buf_size);

#endif
