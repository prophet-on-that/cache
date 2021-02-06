#ifndef _CONN_H
#define _CONN_H

#include "message.h"
#include "hash_table.h"

/* Handle message, returning response message */
Message *handle_msg(Message *msg, HashTable *ht);

#endif
