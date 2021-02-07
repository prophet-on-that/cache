#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "hash_table.h"

typedef uint32_t MessageSize;

typedef struct MessageGet {
  Key key;
} MessageGet;

typedef struct MessageGetResp {
  Val *val;
} MessageGetResp;

typedef struct MessagePut {
  Key key;
  Val val;
} MessagePut;

typedef struct MessagePutResp {
  bool is_update;
} MessagePutResp;

typedef uint8_t MessageType;

typedef union MessageUnion {
  MessageGet get;
  MessagePut put;
  MessageGetResp get_resp;
  MessagePutResp put_resp;
} MessageUnion;

typedef struct Message {
  MessageType type;
  MessageUnion message;
} Message;

#define MESSAGE_TYPE_GET 0
#define MESSAGE_TYPE_PUT 1
#define MESSAGE_TYPE_GET_RESP 2
#define MESSAGE_TYPE_PUT_RESP 2

uint8_t *serialise_message(Message *msg, size_t *buf_size);

Message *deserialise_message(uint8_t *buf, size_t buf_size);

#endif
