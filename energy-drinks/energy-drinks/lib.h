#ifndef LIB_H
#define LIB_H

#include <sys/types.h>

#define BUFFER_SIZE 1024

struct error_info {
    char error_option;
    char error_arg[BUFFER_SIZE];
};

extern struct error_info error_info;

typedef enum error {
    SUCCESS = 0,
    INVALID_ARGUMENT_VALUE,
    ARGUMENT_OUT_OF_RANGE,
    INVALID_OPTION,
    UNEXPECTED_ARGUMENT,
    INSUFFICIENT_RESOURCES,
} error;

int safe_parse_ulong_option(char flag, ulong *result);

int process_error(char *argv[], error error);

#endif // LIB_H
