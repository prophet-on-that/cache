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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
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

  Message *msg = malloc(sizeof(Message));
  msg->type = GET;
  msg->message.get.key.key_size = 1;
  msg->message.get.key.key = malloc(1);
  msg->message.get.key.key[0] = 4;
  size_t buf_size;
  uint8_t *buf = serialise_message(msg, &buf_size);
  if (send_all(sockfd, buf, &buf_size)) {
    perror("send_all");
  };
  free(buf);
  free(msg);                    /* TODO: need to properly free msg */

  /* TODO: receive response */

	/* if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) { */
	/*     perror("recv"); */
	/*     exit(1); */
	/* } */

	/* buf[numbytes] = '\0'; */

	/* printf("client: received '%s'\n",buf); */

	close(sockfd);

	return 0;
}
