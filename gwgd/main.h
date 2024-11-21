#ifndef MAIN_H
#define MAIN_H

#define INT_BASE 10
#define SAFE_CALL(F) if (process_error((F)) != EXIT_SUCCESS) return EXIT_FAILURE;
#define SAFE_CALL_FROM_THREAD(F) if (process_error((F)) != EXIT_SUCCESS) return NULL;
#define BACKLOG 5

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

int safe_parse_ulong_option(char flag, unsigned long *result);

int parse_flags(int argc, char **argv);

int process_error(enum error error);

int server(void);

void *run_game(void *args);

#endif //MAIN_H
