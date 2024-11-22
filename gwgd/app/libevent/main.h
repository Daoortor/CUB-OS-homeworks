#ifndef MAIN_H
#define MAIN_H

#include "../../lib/lib.h"
#include <event2/event.h>
#include <event2/visibility.h>
#include <event2/event.h>

enum server_mode {
	BLOCKING,
	THREAD,
	FORKING,
};

struct config {
	enum server_mode server_mode;
	unsigned long port;

	char error_option;
	char error_arg[BUFFER_SIZE];

	char program_name[BUFFER_SIZE];
};

struct sockfd_config {
	int sockfd;
	struct sockaddr_in servaddr;
	struct sockaddr_in cli;
	socklen_t len;
};

struct talk_args {
	int connfd;
};

int parse_flags(int argc, char **argv);

int server(void);

void cb_func(evutil_socket_t fd, short flags, void *arg);

int init_socket(struct sockfd_config *conf, int domain);

int register_listening_sockets();

void *run_game(void *args);

#endif //MAIN_H
