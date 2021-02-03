#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include "message.h"
#include "hash_table.h"

/* Write message size to buf, returning number of bytes written */
int write_message_size(uint8_t *buf, MessageSize msg_size) {
  *(uint32_t *)buf = htonl(msg_size);
  return sizeof(MessageSize);
}

int write_message_type(uint8_t *buf, MessageType msg_type) {
  *buf = msg_type;
  return sizeof(MessageType);
}

int write_key(uint8_t *buf, Key *key) {
  *(KeySize *)buf = key->key_size;
  memcpy(buf + sizeof(KeySize), key->key, key->key_size);
  return key->key_size + sizeof(KeySize);
}

int write_val(uint8_t *buf, Val *val) {
  *(ValSize *)buf = htons(val->val_size);
  memcpy(buf + sizeof(ValSize), val->val, val->val_size);
  return val->val_size + sizeof(ValSize);
}

/* Allocate buffer to serialise a message in network byte order,
   prepending message size. Stores buffer size in BUF_SIZE. */
uint8_t *serialise_message(Message *msg, size_t *buf_size) {
  MessageSize msg_size;
  uint8_t *buf;
  int offset;
  switch (msg->type) {
  case MESSAGE_TYPE_GET:
    msg_size = key_size(&msg->message.get.key) + sizeof(MessageType);
    buf = malloc(msg_size + sizeof(MessageSize));
    offset = write_message_size(buf, msg_size);
    offset += write_message_type(buf + offset, msg->type);
    write_key(buf + offset, &msg->message.get.key);
    break;
  case MESSAGE_TYPE_PUT:
    msg_size = key_size(&msg->message.put.key) + val_size(&msg->message.put.val) + sizeof(MessageType);
    buf = malloc(msg_size + sizeof(MessageSize));
    offset = write_message_size(buf, msg_size);
    offset += write_message_type(buf + offset, msg->type);
    offset += write_key(buf + offset, &msg->message.put.key);
    write_val(buf + offset, &msg->message.put.val);
    break;
  default:
    error(-1, 0, "Unrecognised message type: %d", msg->type);
  };
  *buf_size = msg_size + sizeof(MessageSize);
  return buf;
}

/* Read key from buffer into KEY, returning bytes read. */
int deserialise_key(uint8_t *buf, Key *key) {
  key->key_size = *(KeySize *)buf;
  key->key = malloc(key->key_size);
  memcpy(key->key, buf + sizeof(KeySize), key->key_size);
  return sizeof(KeySize) + key->key_size;
}

/* Read val from buffer into VAL, returning bytes read. */
int deserialise_val(uint8_t *buf, Val *val) {
  val->val_size = ntohs(*(ValSize *)buf);
  val->val = malloc(val->val_size);
  memcpy(val->val, buf + sizeof(ValSize), val->val_size);
  return sizeof(ValSize) + val->val_size;
}

/* Deserialise a message (excluding MessageSize header) */
Message *deserialise_message(uint8_t *buf, size_t buf_size) {
  MessageType msg_type = buf[0];
  int offset = sizeof(MessageType);
  Message *msg = malloc(sizeof(Message));
  msg->type = msg_type;
  switch (msg_type) {
  case MESSAGE_TYPE_GET:
    deserialise_key(buf + offset, &msg->message.get.key);
    break;
  case MESSAGE_TYPE_PUT:
    offset += deserialise_key(buf + offset, &msg->message.put.key);
    deserialise_val(buf + offset, &msg->message.put.val);
    break;
  default:
    error(0, 0, "Unrecognised message type: %d", msg_type);
    free(msg);
    break;
  };
  return msg;
}
