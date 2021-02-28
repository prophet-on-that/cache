#ifndef _CONN_H
#define _CONN_H

#include "message.h"
#include "hash_table.h"

/* A client connection */
typedef struct Conn {
  MessageSize msg_size;
  uint8_t *msg_buf;
  size_t bytes_received;      /* Running count of buffer allocated */
} Conn;

void
init_conn(Conn *conn);

/* Handle message, returning response message */
Message *
out_handle_msg(Message *msg, HashTable *ht);

Message *
out_recv_msg(Conn *conn, size_t buf_size, uint8_t *buf, size_t *bytes_read);

int
send_all(int socketfd, uint8_t *buf, size_t *len);

#endif
