/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../lib/message.h"
#include "../lib/hash_table.h"
#include "../lib/conn.h"

#define PORT "9034" // the port client will be connecting to

Message *out_receive_msg(int sockfd) {
  size_t buf_size = 128;
  uint8_t *recv_buf = malloc(buf_size);
  Conn conn;
  init_conn(&conn);
  int recv_bytes;
  size_t processed_bytes;
  Message *msg = NULL;
  for (;;) {
    if ((recv_bytes = recv(sockfd, recv_buf, buf_size, 0)) == -1) {
	    perror("recv");
      goto cleanup;
    }
    uint8_t *buf_pos = recv_buf;
    for (;;) {
      msg = out_recv_msg(&conn, recv_bytes, buf_pos, &processed_bytes);
      if (msg)
        goto cleanup;
      buf_pos += processed_bytes;
      if (buf_pos >= recv_buf + recv_bytes)
        break;                  /* Wait on further messages from the network */
    }
  }
 cleanup:
  free(recv_buf);
  return msg;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

Key *out_read_key() {
  char *buf = NULL;
  size_t buf_size = 0;
  Key *key = NULL;
  int key_size;

  printf("key> ");
  key_size = getline(&buf, &buf_size, stdin);
  /* TODO: check for too long key */
  if (key_size < 0) {
      perror("out_read_key");
  } else if (key_size - 1 == 0) {
      printf("Zero-length key invalid\n");
  } else {
    key = malloc(sizeof(Key));
    key->key_size = (KeySize)key_size - 1;
    key->key = malloc(key_size - 1);
    memcpy(key->key, buf, key_size - 1);
  }
  free(buf);
  return key;
}

Val *out_read_val() {
  char *buf = NULL;
  size_t buf_size = 0;
  Val *val = NULL;
  int val_size;

  printf("val> ");
  val_size = getline(&buf, &buf_size, stdin);
  /* TODO: check for too long val */
  if (val_size < 0) {
      perror("out_read_val");
  } else if (val_size - 1 == 0) {
      printf("Zero-length val invalid\n");
  } else {
    val = malloc(sizeof(Val));
    val->val_size = (ValSize)val_size - 1;
    val->val = malloc(val_size - 1);
    memcpy(val->val, buf, val_size - 1);
  }
  free(buf);
  return val;
}

void handle_get(int sockfd, Key *take_key) {
  Message *msg;
  size_t buf_size;
  uint8_t *buf;
  Val *val;
  bool error = false;

  /* Send message */
  msg = malloc(sizeof(Message));
  msg->type = GET;
  msg->message.get.key.key_size = take_key->key_size;
  msg->message.get.key.key = take_key->key;
  free(take_key);

  buf = out_serialise_message(msg, &buf_size);
  if (send_all(sockfd, buf, &buf_size)) {
    perror("handle_get:sendall");
    error = true;
  };
  free(buf);
  free_message(msg);

  if (error)
    return;

  /* Receive response */
  msg = out_receive_msg(sockfd);
  if (msg) {
    if (msg->type == GET_RESP) {
      val = msg->message.get_resp.val;
      if (val) {
        printf("Value: ");
        fwrite(val->val, 1, val->val_size, stdout);
        printf("\n");
      } else
        printf("Value not found\n");
    } else
      printf("Unexpected message type: %d\n", msg->type);
  } else
    printf("Error receiving message\n");

  free_message(msg);
}

void handle_put(int sockfd, Key *take_key, Val *take_val) {
  Message *msg;
  size_t buf_size;
  uint8_t *buf;
  bool error = false;

  /* Send message */
  msg = malloc(sizeof(Message));
  msg->type = PUT;
  msg->message.put.key.key_size = take_key->key_size;
  msg->message.put.key.key = take_key->key;
  msg->message.put.val.val_size = take_val->val_size;
  msg->message.put.val.val = take_val->val;
  free(take_key);
  free(take_val);
  buf = out_serialise_message(msg, &buf_size);
  if (send_all(sockfd, buf, &buf_size)) {
    perror("handle_get:sendall");
    error = true;
  };
  free(buf);
  free_message(msg);

  if (error)
    return;

  /* Receive response */
  msg = out_receive_msg(sockfd);
  if (msg) {
    if (msg->type == PUT_RESP) {
      if (msg->message.put_resp.is_update)
        printf("Value updated\n");
      else
        printf("Value added\n");
    } else
      printf("Unexpected message type: %d\n", msg->type);
  } else
    printf("Error receiving message\n");

  free_message(msg);
}

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
    fprintf(stderr,"usage: client hostname\n");
    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

  for (;;) {
    printf("get/put> ");

    char *cmd = NULL;
    size_t cmd_buf_size = 0;
    int cmd_size = getline(&cmd, &cmd_buf_size, stdin);
    Key *key;
    Val *val;
    size_t key_buf_size, val_buf_size;
    if (cmd_size == -1) {
      perror("getline");
      exit(1);
    }
    cmd[cmd_size - 1] = '\0';   /* Replace newline */
    if (!strcmp(cmd, "get")) {
      /* Handle get */
      key = out_read_key();
      if (!key)
        continue;
      handle_get(sockfd, key);
      /* KEY now invalid */
    } else if (!strcmp(cmd, "put")) {
      /* Handle put */
      key = out_read_key();
      if (!key)
        continue;
      val = out_read_val();
      if (!val)
        continue;
      handle_put(sockfd, key, val);
      /* KEY and VAL now invalid */
    } else
      printf("Unrecognised command\n");
    free(cmd);
  }

	close(sockfd);

	return 0;
}
