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

MessageSize get_message_size(Message *msg) {
  MessageSize s;
  switch (msg->type) {
  case GET:
    s = key_size(&msg->message.get.key);
    break;
  case PUT:
    s = key_size(&msg->message.put.key) + val_size(&msg->message.put.val);
    break;
  case GET_RESP:
    s = msg->message.get_resp.val != NULL ? val_size(msg->message.get_resp.val) : 0;
    break;
  case PUT_RESP:
    s = 1;
    break;
  default:
    error(-1, 0, "Unrecognised message type: %d", msg->type);
  }
  return s + sizeof(MessageType);
}

/* Allocate buffer to serialise a message in network byte order,
   prepending message size. Stores buffer size in BUF_SIZE. */
uint8_t *serialise_message(Message *msg, size_t *buf_size) {
  MessageSize msg_size = get_message_size(msg);
  uint8_t *buf = malloc(msg_size + sizeof(MessageSize));
  int offset = write_message_size(buf, msg_size);
  offset += write_message_type(buf + offset, msg->type);
  switch (msg->type) {
  case GET:
    write_key(buf + offset, &msg->message.get.key);
    break;
  case PUT:
    offset += write_key(buf + offset, &msg->message.put.key);
    write_val(buf + offset, &msg->message.put.val);
    break;
  case GET_RESP:
    /* If VAL is NULL, write nothing */
    if (msg->message.get_resp.val != NULL)
      write_val(buf + offset, msg->message.get_resp.val);
    break;
  case PUT_RESP:
    buf[offset] = msg->message.put_resp.is_update;
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
  size_t offset = sizeof(MessageType);
  Message *msg = malloc(sizeof(Message));
  msg->type = msg_type;
  switch (msg_type) {
  case GET:
    deserialise_key(buf + offset, &msg->message.get.key);
    break;
  case PUT:
    offset += deserialise_key(buf + offset, &msg->message.put.key);
    deserialise_val(buf + offset, &msg->message.put.val);
    break;
  case GET_RESP:
    if (offset < buf_size) {
      msg->message.get_resp.val = malloc(sizeof(Val));
      deserialise_val(buf + offset, msg->message.get_resp.val);
    } else
      msg->message.get_resp.val = NULL;
    break;
  case PUT_RESP:
    msg->message.put_resp.is_update = buf[offset];
    break;
  default:
    error(0, 0, "Unrecognised message type: %d", msg_type);
    free(msg);
    msg = NULL;
    break;
  };
  return msg;
}

void
free_message(Message *msg) {
  switch(msg->type) {
  case GET:
    free(msg->message.get.key.key);
    break;
  case PUT:
    free(msg->message.put.key.key);
    free(msg->message.put.val.val);
    break;
  case GET_RESP:
    free_val(msg->message.get_resp.val);
    break;
  }
  free(msg);
};
