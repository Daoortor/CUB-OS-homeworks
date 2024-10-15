#ifndef PRISONERS_H
#define PRISONERS_H

#include <stdbool.h>

#define BUFFER_SIZE 1024
#define SAFE_CALL(F) if (process_error(argv, (F)) != EXIT_SUCCESS) return EXIT_FAILURE;

enum error {
    SUCCESS = 0,
    INVALID_ARGUMENT_VALUE,
    ARGUMENT_OUT_OF_RANGE,
    INVALID_OPTION,
    UNEXPECTED_ARGUMENT,
    INSUFFICIENT_RESOURCES,
};

enum strategy {
    RANDOM_GLOBAL,
    RANDOM_DRAWER,
    SMART_GLOBAL,
    SMART_DRAWER,
};

struct named_strategy {
    const char *name;
    enum strategy strategy;
};

const struct named_strategy strategies[] = {
    {"random_global", RANDOM_GLOBAL},
    {"random_drawer", RANDOM_DRAWER},
    {"smart_global", SMART_GLOBAL},
    {"smart_drawer", SMART_DRAWER}
};

struct config {
    unsigned long n;
    unsigned long s;
    char error_option;
    char error_arg[BUFFER_SIZE];
};

struct args {
    unsigned long index;
    bool use_smart_strategy;
    bool use_global_lock;
    bool *result;
};

int safe_parse_ulong_option(char flag, unsigned long *result);

int parse_flags(int argc, char **argv);

int process_error(char *argv[], enum error error);

void init_drawers();

int next_drawer(int index, int seed, bool use_smart_strategy);

void *run_prisoner(void *args_ptr);

int simulate(enum strategy strategy, int *success_count, double *time_milliseconds);

#endif //PRISONERS_H
