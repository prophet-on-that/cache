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

enum MessageType {
  GET,
  PUT,
  GET_RESP,
  PUT_RESP
} __attribute__ ((__packed__));

typedef enum MessageType MessageType;

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

uint8_t *out_serialise_message(Message *msg, size_t *buf_size);

Message *out_deserialise_message(uint8_t *buf, size_t buf_size);

void
free_message(Message *msg);

#endif
