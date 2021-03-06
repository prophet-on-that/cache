#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "../lib/conn.h"
#include "../lib/hash_table.h"

#define PORT "9034"   // Port we're listening on

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
  int listener;     // Listening socket descriptor
  int yes=1;        // For setsockopt() SO_REUSEADDR, below
  int rv;

  struct addrinfo hints, *ai, *p;

  // Get us a socket and bind it
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
    fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
    exit(1);
  }

  for(p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) {
      continue;
    }

    // Lose the pesky "address already in use" error message
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
      close(listener);
      continue;
    }

    break;
  }

  // If we got here, it means we didn't get bound
  if (p == NULL) {
    return -1;
  }

  freeaddrinfo(ai); // All done with this

  // Listen
  if (listen(listener, 10) == -1) {
    return -1;
  }

  return listener;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
  // If we don't have room, add more space in the pfds array
  if (*fd_count == *fd_size) {
    *fd_size *= 2; // Double it

    *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
  }

  (*pfds)[*fd_count].fd = newfd;
  (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

  (*fd_count)++;
}

void add_to_conns(Conn **conns, size_t *conns_size, unsigned int conn_count) {
  if (conn_count == *conns_size) {
    *conns_size *= 2;
    *conns = realloc(*conns, sizeof(Conn) * *conns_size);
  }
  init_conn(*conns + conn_count);
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
  // Copy the one from the end over this one
  pfds[i] = pfds[*fd_count-1];

  (*fd_count)--;
}

void del_from_conns(Conn *conns, int i, unsigned int conn_count) {
  /* Copy end conn over this one */
  conns[i] = conns[conn_count - 1];
}

// Main
int main(void)
{
  int listener;     // Listening socket descriptor

  int newfd;        // Newly accept()ed socket descriptor
  struct sockaddr_storage remoteaddr; // Client address
  socklen_t addrlen;

  uint8_t buf[256];    // Buffer for client data

  char remoteIP[INET6_ADDRSTRLEN];

  // Start off with room for 5 connections
  // (We'll realloc as necessary)
  int fd_count = 0;
  int fd_size = 5;
  struct pollfd *pfds = malloc(sizeof *pfds * fd_size);
  size_t conns_size = fd_size - 1;
  Conn *conns = malloc(sizeof(Conn) * conns_size);
  HashTable *ht = create_hash_table(128); /* TODO: update size */

  // Set up and get a listening socket
  listener = get_listener_socket();

  if (listener == -1) {
    fprintf(stderr, "error getting listening socket\n");
    exit(1);
  }

  // Add the listener to set
  pfds[0].fd = listener;
  pfds[0].events = POLLIN; // Report ready to read on incoming connection

  fd_count = 1; // For the listener

  // Main loop
  for(;;) {
    int poll_count = poll(pfds, fd_count, -1);

    if (poll_count == -1) {
      perror("poll");
      exit(1);
    }

    // Run through the existing connections looking for data to read
    for(int i = 0; i < fd_count; i++) {

      // Check if someone's ready to read
      if (pfds[i].revents & POLLIN) { // We got one!!

        if (pfds[i].fd == listener) {
          // If listener is ready to read, handle new connection

          addrlen = sizeof remoteaddr;
          newfd = accept(listener,
                         (struct sockaddr *)&remoteaddr,
                         &addrlen);

          if (newfd == -1) {
            perror("accept");
          } else {
            add_to_conns(&conns, &conns_size, fd_count - 1);
            add_to_pfds(&pfds, newfd, &fd_count, &fd_size);

            printf("pollserver: new connection from %s on "
                   "socket %d\n",
                   inet_ntop(remoteaddr.ss_family,
                             get_in_addr((struct sockaddr*)&remoteaddr),
                             remoteIP, INET6_ADDRSTRLEN),
                   newfd);
          }
        } else {
          // If not the listener, we're just a regular client
          int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

          int sender_fd = pfds[i].fd;

          if (nbytes <= 0) {
            // Got error or connection closed by client
            if (nbytes == 0) {
              // Connection closed
              printf("pollserver: socket %d hung up\n", sender_fd);
            } else {
              perror("recv");
            }

            close(pfds[i].fd); // Bye!

            del_from_conns(conns, i - 1, fd_count - 1);
            del_from_pfds(pfds, i, &fd_count);
          } else {
            size_t bytes_read;
            uint8_t *buf_pos = buf;
            for (;;) {
              Message *msg = out_recv_msg(conns + i - 1, nbytes, buf_pos, &bytes_read);
              if (msg) {
                Message *resp = out_handle_msg(msg, ht);
                if (resp) {
                  size_t resp_buf_size;
                  uint8_t *resp_buf = out_serialise_message(resp, &resp_buf_size);
                  if (send_all(sender_fd, resp_buf, &resp_buf_size))
                    perror("send_all");
                  free(resp_buf);
                  free_message(resp);
                }
                free_message(msg);
              }
              buf_pos += bytes_read;
              if (buf_pos >= buf + nbytes)
                break;
            }
          }
        } // END handle data from client
      } // END got ready-to-read from poll()
    } // END looping through file descriptors
  } // END for(;;)

  return 0;
}
