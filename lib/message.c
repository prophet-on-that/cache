#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include "message.h"
#include "hash_table.h"

/* Allocate buffer to serialise a message in network byte order,
   prepending message size. Stores buffer size in BUF_SIZE. */
uint8_t *serialise_message(Message *msg, size_t *buf_size) {
  MessageSize msg_size;
  uint8_t *buf;
  int offset;
  switch (msg->type) {
  case MessageTypeGet:
    msg_size = key_size(msg->message.get.key) + sizeof(MessageType);
    buf = malloc(msg_size + sizeof(MessageSize));
    /* Write message size */
    *(uint32_t *)buf = htonl(msg_size);
    offset = sizeof(MessageSize);
    /* Write message type */
    *(buf + offset) = msg->type;
    offset += sizeof(MessageType);
    /* Write key size */
    *(KeySize *)(buf + offset) = msg->message.get.key->key_size;
    offset += sizeof(KeySize);
    /* Write key */
    memcpy(buf + offset, msg->message.get.key->key, msg->message.get.key->key_size);
    break;
  /* case MessageTypePut: */
  /*   break; */
  default:
    error(-1, 0, "Unrecognised message type: %d", msg->type);
  };
  *buf_size = msg_size + sizeof(MessageSize);
  return buf;
}

Message *deserialise_message(uint8_t *buf, size_t buf_size) {
  MessageType msg_type = buf[0];
  int offset = sizeof(MessageType);
  Message *msg = malloc(sizeof(Message));
  Key *key;
  msg->type = msg_type;
  switch (msg_type) {
  case MessageTypeGet:
    key = malloc(sizeof(Key));
    key->key_size = *(KeySize *)(buf + offset);
    offset += sizeof(KeySize);
    key->key = malloc(key->key_size);
    memcpy(key->key, buf + offset, key->key_size);
    msg->message.get.key = key;
    break;
  default:
    error(0, 0, "Unrecognised message type: %d", msg_type);
    free(msg);
    break;
  };
  return msg;
}
