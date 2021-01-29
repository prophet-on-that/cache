#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "hash_table.h"

typedef uint32_t MessageSize;

/* A client connection */
typedef struct Conn {
  MessageSize msg_size;
  uint8_t *msg_buf;
  size_t bytes_received;      /* Running count of buffer allocated */
} Conn;

void init_conn(Conn *conn) {
  conn->msg_size = 0;
  conn->msg_buf = NULL;
  conn->bytes_received = 0;
}

int min(int a, int b) {
  return a < b ? a : b;
}

#define MSG_PUT 0

void handle_msg(Conn *conn, hash_table_t *ht) {
  struct key_t key;
  val_t val;

  switch(conn->msg_buf[0]) {
  case MSG_PUT:
    key.key_size = ntohs(conn->msg_buf[1]);
    key.key = conn->msg_buf + 1 + sizeof(key_size_t);
    val.val_size = ntohs(conn->msg_buf[1 + sizeof(key_size_t) + key.key_size]);
    val.val = conn->msg_buf + 1 + sizeof(key_size_t) + key.key_size + sizeof(val_size_t);
    hash_table_put(ht, &key, &val);
    /* TODO: send response */
    break;
  default:
    /* TODO: send error payload */
    break;
  }
}

/* Handle incoming data for a client. Returns bytes consumed in
   buffer. */
size_t recv_msg(Conn *conn, hash_table_t *ht, size_t buf_size, uint8_t *buf) {
  size_t outstanding_bytes, bytes_read;
  if (conn->msg_size) {
    outstanding_bytes = conn->msg_size - conn->bytes_received;
    bytes_read = min(buf_size, outstanding_bytes);
    memcpy(conn->msg_buf + conn->bytes_received, buf, bytes_read);
    conn->bytes_received += bytes_read;
    assert(conn->bytes_received <= conn->msg_size);
    if (conn->bytes_received == conn->msg_size) {
      /* TODO: Parse message and reset conn */
      handle_msg(conn, ht);
      conn->msg_size = 0;
      free(conn->msg_buf);
      conn->bytes_received = 0;
    }
  } else {
    /* No size information yet message */
    if (!conn->bytes_received)
      conn->msg_buf = malloc(sizeof(MessageSize));
    /* This logic handles where less than sizeof(MessageSize) bytes is
       received by storing partial bytes in the buffer. */
    outstanding_bytes = sizeof(MessageSize) - conn->bytes_received;
    bytes_read = min(buf_size, outstanding_bytes);
    memcpy(conn->msg_buf + conn->bytes_received, buf, bytes_read);
    conn->bytes_received += bytes_read;
    assert(conn->bytes_received <= sizeof(MessageSize));
    if (conn->bytes_received == sizeof(MessageSize)) {
      conn->msg_size = ntohl((uint32_t)conn->msg_buf[0]);
      conn->msg_buf = realloc(conn->msg_buf, conn->msg_size);
      conn->bytes_received = 0;
    }
  }
  return bytes_read;
}
