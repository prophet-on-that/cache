#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <error.h>
#include "hash_table.h"
#include "message.h"
#include "conn.h"

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

/* Handle message, returning response message */
Message *handle_msg(Message *msg, HashTable *ht) {
  Message *resp = malloc(sizeof(Message));
  Val *val;
  bool is_update;
  switch (msg->type) {
  case GET:
    val = hash_table_get(ht, &msg->message.get.key);
    resp->type = GET_RESP;
    if (val != NULL) {
      /* Copy val to resp */
      resp->message.get_resp.val = malloc(sizeof(Val));
      resp->message.get_resp.val->val_size = val->val_size;
      resp->message.get_resp.val->val = malloc(val->val_size);
      memcpy(resp->message.get_resp.val->val, val->val, val->val_size);
    } else {
        resp->message.get_resp.val = NULL;
    }
    break;
  case PUT:
    is_update = hash_table_put(ht, &msg->message.put.key, &msg->message.put.val);
    resp->type = PUT_RESP;
    resp->message.put_resp.is_update = is_update;
    break;
  default:
    free(resp);
    error(0, 0, "Unhandled message type %d", msg->type);
  };
  return resp;
}

/* Handle incoming data for a client. Returns bytes consumed in
   buffer. */
size_t recv_msg(Conn *conn, HashTable *ht, size_t buf_size, uint8_t *buf) {
  size_t outstanding_bytes, bytes_read;
  if (conn->msg_size) {
    outstanding_bytes = conn->msg_size - conn->bytes_received;
    bytes_read = min(buf_size, outstanding_bytes);
    memcpy(conn->msg_buf + conn->bytes_received, buf, bytes_read);
    conn->bytes_received += bytes_read;
    assert(conn->bytes_received <= conn->msg_size);
    if (conn->bytes_received == conn->msg_size) {
      /* handle_msg(conn, ht); */
      /* TODO */
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
