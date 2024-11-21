#ifndef MAIN_H
#define MAIN_H

#include "../../lib/lib.h"

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

struct talk_args {
    int connfd;
};

int parse_flags(int argc, char **argv);

int server(void);

void *run_game(void *args);

#endif //MAIN_H
