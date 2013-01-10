#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>

#include "chatgui.h"

#define BUFSIZE 8192

static void
error_exit(int ret_value, const char* msg) {
  fprintf(stderr, "error: %s!\n", msg);
  exit(ret_value);
}

static void
print_usage(const char* app_name) {
  fprintf(stderr, "usage: %s server-name port\n", app_name);
  exit(EXIT_FAILURE);
}

static int
connect_to_server(const char* server, const char* port, int* out_fd) {
  struct addrinfo hints, *res, *rp;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  int gai_ret = getaddrinfo(server, port, &hints, &res);
  if (gai_ret != 0) {
    return gai_ret;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    *out_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (*out_fd == -1) {
      continue;
    }
    if (connect(*out_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }
    close(*out_fd);
  }
  freeaddrinfo(res);
  return rp == NULL ? -1 : 0;
}

static int
flush_to(int fd, const char* buf, int len) {
  int written_out = 0;
  int bytes_written = 0;

  do {
    bytes_written = write(fd, buf + written_out, len - written_out);
    written_out += bytes_written;
  } while (bytes_written > 0 && written_out < len);
  return bytes_written <= 0 ? -1 : 0;
}

static int
read_write_to_gui(int sfd, int guifd) {
  char buf[BUFSIZE];
  int bytes_read;
  if ((bytes_read = read(sfd, buf, BUFSIZE)) <= 0) {
    return -1;
  }

#ifdef DEBUG
  flush_to(STDOUT_FILENO, buf, bytes_read);
#endif
  return flush_to(guifd, buf, bytes_read);
}

static int
write_to_server(int sfd, int guifd) {
  char buf[BUFSIZE];
  int bytes_read;
  if ((bytes_read = read(guifd, buf, BUFSIZE)) <= 0) {
    return -1;

  }
#ifdef DEBUG
  flush_to(STDOUT_FILENO, buf, bytes_read);
#endif
  return flush_to(sfd, buf, bytes_read);
}

int
main(int argc, char* argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
  }

  char* server = argv[1];
  char* port = argv[2];
  int infd, outfd, sfd;

  if (gui_start(&infd, &outfd) < 0) {
    error_exit(EXIT_FAILURE, "failed to start gui");
  }

  if (connect_to_server(server, port, &sfd) != 0) {
    error_exit(EXIT_FAILURE, "failed to connect to server");
  }

  struct pollfd fds[2];
  fds[0].fd = sfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  fds[1].fd = infd;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  while (true) {
    poll(fds, 2, -1);

    if (fds[0].revents != 0) {
      if (read_write_to_gui(sfd, outfd) != 0) {
	close(sfd);
	close(infd);
	close(outfd);
	error_exit(EXIT_FAILURE, "server closed connection");
      }
    }

    if (fds[1].revents != 0) {
      if (write_to_server(sfd, infd) != 0) {
	close(sfd);
	close(infd);
	close(outfd);
	error_exit(EXIT_FAILURE, "write failed");
      }
    }
  }

  return EXIT_SUCCESS;
}
